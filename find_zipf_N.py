#!/usr/bin/env python

import sys
import math

BYTES_PER_ITEM = 56

def find_zipf_N(cache_size, zipf_alpha, percentile):
    cache_item_count = int(cache_size / BYTES_PER_ITEM)

    sys.stderr.write('cache_size = %d bytes (%d items)\n' % (cache_size, cache_item_count))
    sys.stderr.write('zipf_alpha = %f\n' % zipf_alpha)
    sys.stderr.write('percentile = %f\n' % percentile)

    # Find N such that
    # percentile = sum_{i=1}{cache_item_count}{1/i**zipf_alpha} /
    #              sum_{i=1}{N}{1/i**zipf_alpha}

    numerator = 0.
    for i in range(1, cache_item_count + 1):
        numerator += 1. / (i ** zipf_alpha)

    rhs = numerator / percentile
    #print('rhs = %f' % rhs)

    lhs = 0.
    prevN = 0.
    N = 1.
    while lhs < rhs:
        for i in range(int(prevN) + 1, int(N) + 1): 
            lhs += 1. / (i ** zipf_alpha)
        prevN = N
        # Accurate but slow
        N += 1.
        # Inaccurate but fast
        #N *= 1.001
        #print('lhs = %f' % lhs)

    sys.stderr.write('found N = %d\n' % prevN)

    print('%d' % prevN)
    return prevN


def test_find_zipf_N():
    assert find_zipf_N(100000, 0.0, 1.0) == 1785
    
#test_find_zipf_N()


if __name__ == '__main__':
    if len(sys.argv) != 4:
        print('%s CACHE-SIZE ZIPF-ALPHA PERCENTILE' % sys.argv[0])
        sys.exit(1)
    cache_size = int(sys.argv[1])
    zipf_alpha = float(sys.argv[2])
    percentile = float(sys.argv[3])
    find_zipf_N(cache_size, zipf_alpha, percentile)
