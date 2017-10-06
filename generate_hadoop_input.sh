#!/bin/sh
mkdir -p text
mkdir -p text/small
mkdir -p text/medium
rm -rf text/small/*
rm -rf text/medium/*
perl -e 'print "Hello\nWorld\n" x 34952533' > text/small/test.txt
perl -e 'print "Hello\nWorld\n" x 52428800' > text/medium/file1.txt
cp text/medium/file1.txt text/medium/file2.txt
cp text/medium/file1.txt text/medium/file3.txt
