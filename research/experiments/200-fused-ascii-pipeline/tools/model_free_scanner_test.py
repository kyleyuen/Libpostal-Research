#!/usr/bin/env python3
"""Extract exp200's exact helper and compare its scanner spans to tokenize(lower(input))."""
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[4]
SRC = ROOT / "research/experiments/200-fused-ascii-pipeline/sources/address_parser.c"
OUT = ROOT / "research/experiments/200-fused-ascii-pipeline/raw/scanner-helper-test.c"


def extract_function(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    raise ValueError(f"unterminated function: {signature}")


source = SRC.read_text()
helper = extract_function(source, "static bool ap_fused_ascii_normalize_tokenize(")
scaffold = r'''#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

'''
tests = r'''
static int check_one(const char *input) {
    size_t n=strlen(input); char *lower=malloc(n+1); if(!lower) return 1;
    for(size_t i=0;i<n;i++) { unsigned char c=(unsigned char)input[i]; lower[i]=(c>='A'&&c<='Z')?(char)(c+('a'-'A')):(char)c; }
    lower[n]='\0';
    token_array *got=token_array_new(); if(!got) { free(lower); return 1; }
    char *normalized=NULL;
    if(!ap_fused_ascii_normalize_tokenize(input,&normalized,got)) { fprintf(stderr,"unexpected fallback for in-domain input '%s'\n",input); token_array_destroy(got); free(lower); return 1; }
    if(strcmp(normalized,lower)!=0) { fprintf(stderr,"normalized bytes mismatch\n"); free(normalized); token_array_destroy(got); free(lower); return 1; }
    token_array *expected=tokenize(normalized);
    int bad=expected==NULL || expected->n!=got->n;
    if(!bad) for(size_t i=0;i<got->n;i++) if(got->a[i].offset!=expected->a[i].offset || got->a[i].len!=expected->a[i].len || got->a[i].type!=expected->a[i].type) { bad=1; break; }
    if(bad) fprintf(stderr,"scanner type/span mismatch for input length %zu\n",n);
    token_array_destroy(expected); token_array_destroy(got); free(normalized); free(lower); return bad;
}
static int must_fallback(const char *input) {
    token_array *tokens=token_array_new(); char *normalized=NULL; if(!tokens) return 1;
    bool ok=ap_fused_ascii_normalize_tokenize(input,&normalized,tokens);
    int bad=ok || normalized!=NULL || tokens->n!=0;
    if(bad) fprintf(stderr,"unsupported input did not clear/fallback\n");
    token_array_destroy(tokens); free(normalized); return bad;
}
static int exhaustive(char *s, size_t pos, size_t n, const char *alphabet, size_t k) {
    if(pos==n) return check_one(s);
    for(size_t i=0;i<k;i++) { s[pos]=alphabet[i]; int r=exhaustive(s,pos+1,n,alphabet,k); if(r) return r; }
    return 0;
}
int main(void) {
    int bad=0; const char *alphabet="AaZz09 "; char sample[8]={0}; size_t k=strlen(alphabet);
    for(size_t n=1;n<=6;n++) bad |= exhaustive(sample,0,n,alphabet,k);
    // Scanner rules include case-sensitive token classes (Roman numerals, ordinals,
    // scientific notation and common address abbreviations), so compare targeted
    // semantic cases beyond the reduced-alphabet enumeration.
    const char *semantic[] = {
        "I", "i", "IV", "iv", "XII", "xii", "IX", "ix", "XL", "xl", "MCCM", "mccm",
        "1st", "1ST", "2nd", "2ND", "3rd", "3RD", "4th", "4TH", "1e2", "1E2",
        "St", "ST", "st", "PO", "Po", "UK", "Uk", "US", "Us", "NW", "Nw",
        "12 Main St", "12 MAIN ST", "PO Box 4", "PO BOX 4", "IV Main St", "IV MAIN ST",
        "1E2 St NW", "1e2 st nw", "Apt 2B", "APT 2b", NULL
    };
    for(size_t i=0;semantic[i]!=NULL;i++) bad |= check_one(semantic[i]);
    char stress[4097]; for(size_t i=0;i<4096;i++) stress[i]="AbZz09 "[i%7]; stress[4096]='\0'; bad |= check_one(stress);
    bad |= must_fallback(""); bad |= must_fallback("Main\tStreet"); bad |= must_fallback("A+B"); bad |= must_fallback("Caf\xc3\xa9");
    if(bad) return 1;
    puts("ASCII scanner equivalence: every string of length 1..6 over AaZz09-space, targeted case-sensitive scanner semantics, and 4096-byte stress passed; empty/tab/punctuation/UTF-8 fallback cleared temporary state");
    return 0;
}
'''
OUT.write_text(scaffold + helper + "\n\n" + tests)
print(f"generated {OUT}; source={hashlib.sha256(SRC.read_bytes()).hexdigest()}; harness={hashlib.sha256(OUT.read_bytes()).hexdigest()}")
