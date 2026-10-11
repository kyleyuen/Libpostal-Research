# Certified argmax: why a reordered or float32 score computation can return exactly the original labels

This note is written for review. It states the claim, the assumptions and the proof. Experiment 152's runtime test (`src/cert_block.h`) implements
exactly this certificate.

## 1. Setting

For each token t and labels i, j ∈ {0..L−1} (L = 20), the original computes these doubles in a fixed order:
- state scores S_t(j): each is the sum, in feature-emission order, of the weights of the state rows hit at token t;
- state-transition scores ST_t(i, j): the same, for the prev-tag rows;
- transition weights TR(i, j): model constants.

Viterbi (`crf_context_viterbi`) then computes

    α_0(j) = S_0(j)
    score_t(i, j) = fl( fl( α_{t-1}(i) + ST_t(i, j) ) + TR(i, j) )
    b_t(j) = first i with the strictly largest score_t(i, j)        (loop i = 0..L-1, update on `max < score`)
    α_t(j) = fl( max_i score_t(i, j) + S_t(j) )
    last label = first j with the strictly largest α_{T-1}(j); earlier labels follow back-pointers b_t.

Here fl is IEEE-754 round-to-nearest-even. **The output depends only on the labels.** The default API never returns scores.

A *runtime* computes approximations S′, ST′ and TR′ differently:
- **R64:** other summation orders, or rows merged ahead of time;
- **R32:** float32 weights and sums.

It then runs the same recurrence in its own precision and gets α′, score′, b′ and a path p′.

## 2. Per-entry error bound

**Original's own error.** Take an entry with k terms w_1..w_k, exact real sum σ and A = Σ|w_i|. For recursive summation in **any** order with
unit roundoff u (Higham, *Accuracy and Stability of Numerical Algorithms*, §4.2):

    |computed − σ| ≤ γ_{k−1}·A,   γ_n = n·u / (1 − n·u).

So the original's value is within γ_{k−1}(u64)·A of σ.

**Runtime error.**
- **R64:** within γ_{k−1}(u64)·A of σ.
- **R32:** rounding each weight to float32 adds at most u32·|w_i|, so the error is within (u32 + γ_{k−1}(u32)(1+u32))·A.

By the triangle inequality,

    |S′ − S| ≤ c(k)·A,   c(k) = 2γ_{k−1}(u64)                               (R64)
                              = u32 + γ_{k−1}(u32)(1+u32) + γ_{k−1}(u64)    (R32).

**What the runtime uses.**
- A_t = Σ over the token's rows of each row's Σ|w|. This is a model constant per row, and an upper bound of A for every entry of the token.
- k = the number of rows at the token, an upper bound on the terms in any entry.

The same bound is used for ST. In R32 the transition constants get u32·|TR|. Bounds are multiplied by 1.001.

## 3. Propagation through the recurrence

**Claim.** Suppose |α′_{t−1}(i) − α_{t−1}(i)| ≤ D_{t−1} for all i. Define

    ρ_t = 3 (u_runtime + u64) · M_t,   M_t ≥ |every intermediate value of either run at step t|
    δ_t = D_{t−1} + e^P_t + e^TR + ρ_t
    D_t = δ_t + e^S_t + ρ_t

Then |score′_t(i, j) − score_t(i, j)| ≤ δ_t and |α′_t(j) − α_t(j)| ≤ D_t.

**Proof.**
- Each run forms a score with two additions. Each addition contributes the difference of its inputs plus one rounding, which is at most u·M_t.
- max over i is 1-Lipschitz in the sup norm.
- Adding S versus S′ (difference ≤ e^S_t) contributes one more rounding per run.
- D_0 = e^S_0. In R32, D_0 also includes u32·max|S_0| for storing S_0 in float. ∎

**Everything on the right-hand side is available to the runtime:**
- the row abs-sums are model constants;
- the row counts and its own magnitudes are known;
- M_t is built from |α′|, |ST′|, |S′|, |TR| plus D and the e terms.

## 4. The certificate

**Statement.** Let p′ be the runtime path.
- (a) For every t ≥ 1, at node (t, p′_t), let g′_t = score′_t(p′_{t−1}, p′_t) − max_{i ≠ p′_{t−1}} score′_t(i, p′_t). Require **g′_t > 2δ_t**.
- (b) Let the final gap be α′_{T−1}(p′_{T−1}) − max_{j ≠ p′_{T−1}} α′_{T−1}(j). Require it **> 2D_{T−1}**.

If (a) and (b) hold, the original's labels equal p′.

**Proof.**
- **Final label.** By (b) and §3, for every j ≠ p′_{T−1}:

      α_{T−1}(p′_{T−1}) ≥ α′(p′_{T−1}) − D > α′(j) + D ≥ α_{T−1}(j).

  The original's maximum is therefore unique and lies at p′_{T−1}. Its "first strictly largest" rule picks it.
- **Back-pointers.** Suppose the original's label at t equals p′_t. By (a) and §3, for every i ≠ p′_{t−1}:

      score_t(p′_{t−1}, p′_t) ≥ score′ − δ > score′(i) + δ ≥ score_t(i, p′_t).

  So b_t(p′_t) = p′_{t−1}.
- **Induction.** Induction backwards in t gives equality of all labels. The original never reads back-pointers off its own path. ∎

**Ties.** Exact ties have g = 0, so they are never certified. Strict inequality also makes the "first index" tie-break irrelevant whenever the
certificate holds.

## 5. Runtime rule

1. Compute S′, ST′ and the bounds.
2. Run Viterbi and backtrack.
3. Check (a) at the T−1 path nodes (20 candidates each) and (b) once.
4. If any check fails, **recompute the address in the original order** and return those labels.

The output is therefore always the original's. The check costs O(T·L), next to Viterbi's O(T·L²).

## 6. When no certificate is needed

Some runtimes reproduce every S and ST bit for bit, with no certificate needed. This holds whenever the runtime adds each row **in the
original feature-emission order**, whatever the memory layout (records, co-located rows), including:
- adding a whole row with SIMD **across labels**, because that never reorders the additions into one label;
- vectorizing Viterbi across j, because each cell keeps the original's operations and its i-order comparison.

The certificate is required only for float32 storage or for rows merged ahead of time (pre-summed vectors).

## 7. Assumptions

- **Arithmetic.**
  - IEEE-754 binary64/binary32 round-to-nearest, AArch64 default FPCR.
  - No flush-to-zero and no `-ffast-math` on the certified path.
  - No FMA contraction in either score recurrence. Use `-ffp-contract=off`, or verify the generated code.
- **Finite values.** All weights are finite, and every magnitude is far below overflow. The runtime should check both at load.
- **Bound computation.** The bound is computed in double. Its own rounding (a few ulps of a quantity ~1e-12 of the scores) is covered by the 1.001 factor.
