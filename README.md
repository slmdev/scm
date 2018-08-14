# scm
scm - context model - last code update 2011

scm is a simple command line tool to compress individual files using bitwise context modelling.
for every input bit scm creates a set of individual predictions which get mixed together using logistic mixing.

models: ppm, match, record, indirect, sparse, word, sse

using 500mb of memory scm compresses calgary.tar: 3.152.896 bytes to 625.583 bytes (1.587bpb).
