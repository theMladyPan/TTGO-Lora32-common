#include "BigInt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <climits>
#include <cassert>
#include <algorithm>

static uint32_t BITS[] = 
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,   
    0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,   
    0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,   
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000
};

static uint32_t BITS2[] =
{
    0x00FFFFFF, 0x0000FFFF, 0x00FFFFFF
};

static inline uint32_t num(const uint32_t a)
{
    return a/4 + (a%4 ? 1:0); 
}


BigInt::BigInt():
sign(POS),
size(0),
bits(0)
{
}

BigInt::BigInt(int32_t a)
{
    if(a < 0) 
    {
        a = -a;
        sign = NEG;
    }
    else
        sign = POS;

    if(a >> 24)
        size = 4;
    else if(a >> 16)
        size = 3;
    else if(a >> 8)
        size = 2;
    else
        size = 1;
    bits = new uint32_t[1];
    bits[0] = a;
}

BigInt::BigInt(const BigInt &a):
sign(a.sign),
size(a.size)
{
    uint32_t l = num(size);
    bits = new uint32_t[l];
    for(int i = 0; i < (int)l; ++i)
        bits[i] = a.bits[i];
}


BigInt::~BigInt()
{
    if(size)
    {
        delete[] bits;
    }
}

BigInt& BigInt::operator=(const BigInt& a)
{
    sign = a.sign;
    size = a.size;
    uint32_t l = num(size);
    if(bits)
        delete[] bits;
    bits = new uint32_t[l];
    for(int i = 0; i < (int)l; ++i)
        bits[i] = a.bits[i];    
    return *this;
}

void BigInt::importData(uint8_t *data, uint32_t length, bool sign)
{
    this->sign = sign;
    size = length;
    if(bits)
        delete[] bits;
    bits = new uint32_t[num(size)];
    memset(bits, 0, sizeof(uint32_t)*num(size));
    for(int i = 0; i < (int)length; ++i)
        bits[i/4] |= data[length-i-1] <<  ((i%4)*8);
        
    trim();
}

void BigInt::exportData(uint8_t *data, uint32_t length, bool &sign)
{
    assert(isValid() && data != 0);
    
    if(length < size)
        return;
        
    sign = this->sign;
    uint32_t offset = length-size;
    memset(data, 0, offset);
    for(int i = size-1; i >= 0; --i)
        data[offset+size-1-i] = bits[i/4] >> ((i%4)*8);
}

BigInt operator+(const BigInt &a, const BigInt& b)
{
    assert(a.isValid() && b.isValid());

    if(a.sign == POS && b.sign == POS)      // a+b
        return add(a, b);
    if(a.sign == NEG && b.sign == NEG)      // (-a)+(-b) = -(a+b)
        return -add(a, b);
    else if(a.sign == POS)  // a + (-b) = a-b
        return a - (-b);
    else                    // (-a) + b = b-a
        return b - (-a);
}

BigInt& BigInt::operator+=(const BigInt &b)
{
    return (*this = (*this) + b);
}

BigInt& BigInt::operator++()
{
    return (*this += 1);
}

BigInt BigInt::operator++(int)
{
    BigInt t = *this;
    *this += 1;
    return t;
}

BigInt operator-(const BigInt& a, const BigInt& b)
{
    assert(a.isValid() && b.isValid());
    
    if(a.sign == POS && b.sign == POS)
    {
        if(equals(a, b))
            return 0;
        else if(greater(a, b))
            return sub(a, b);
        else
            return -sub(b, a);
    }
    else if(a.sign == NEG && b.sign == NEG)
        return (-b) - (-a);
    else if(a.sign == NEG && b.sign == POS)
        return -add(a, b);
    else 
        return add(a, b);
}

BigInt operator-(const BigInt &a)
{
    assert(a.isValid());
    
    BigInt result = a;
    result.sign = !a.sign;
    return result;
}

BigInt& BigInt::operator-=(const BigInt &b)
{
    return (*this = (*this) - b);
}

BigInt& BigInt::operator--()
{
    return (*this -= 1);
}

BigInt BigInt::operator--(int)
{
    BigInt t = *this;
    *this -= 1;
    return t;
}

BigInt operator*(const BigInt &a, const BigInt& b)
{
    assert(a.isValid() && b.isValid());

    // if a == 0 or b == 0 then result = 0
    if(!a || !b)
        return 0;

    BigInt result;
    if(equals(a, 1))
        result = b;
    else if(equals(b, 1))
        result = a;
    else
        result = mul(a, b);

    if(a.sign == b.sign)
        result.sign = POS;
    else
        result.sign = NEG;
    return result;
}

BigInt& BigInt::operator*=(const BigInt &b)
{
    return (*this = (*this) * b);
}

BigInt operator/(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid() && b != 0);
    
    if(lesser(a, b))
        return 0;

    BigInt result;
    
    if(equals(a, b))
        result = 1;
    else if(equals(b, 1))
        result = a;
    else
        result = div(a, b);
        
    if(a.sign == b.sign)
        result.sign = POS;
    else
        result.sign = NEG;
        
    return result;
}

BigInt& BigInt::operator/=(const BigInt &b)
{
    return (*this = (*this) / b);
} 

BigInt operator>>(const BigInt &a, const uint32_t m)
{
    assert(a.isValid());

    if(m == 0)
        return a;
    if(m/8 >= a.size)
        return 0;
    
    BigInt result;
    result.size = a.size - m/8;
    result.bits = new uint32_t[num(result.size)];
    memset(result.bits, 0, sizeof(uint32_t)*num(result.size));
    uint8_t s = m%32;
    for(uint32_t i = 0; i < num(result.size); ++i)
    {
        if(m/32+i+1 < num(a.size))
            result.bits[i] =  (a.bits[m/32+i+1] << (32-s)) |  (a.bits[m/32+i] >> s);
        else
            result.bits[i] = (a.bits[m/32+i] >> s);
    }
    
    result.trim();
    result.checkZero();
            
    return result;
}

BigInt& BigInt::operator>>=(const uint32_t m)
{
    return (*this = (*this >> m));
}
 
BigInt operator<<(const BigInt& a, const uint32_t m)
{
    assert(a.isValid());

    if(m == 0)
        return a;

    BigInt result;
    
    result.size = m/8 + a.size;
    if((m%32)%8 != 0)
        ++result.size;
    uint32_t l = num(result.size);
    result.bits = new uint32_t[l];
    memset(result.bits, 0, sizeof(uint32_t)*l);
    uint32_t s = m % 32;
    result.bits[m/32] = a.bits[0] << s;
    for(uint32_t i = 1; i < num(a.size); ++i)
        result.bits[m/32+i] = (a.bits[i] << s) | (a.bits[i-1] >> (32-s));
    if(s != 0 && num(result.size) != 1)
        result.bits[num(result.size)-1] |= a.bits[num(a.size)-1] >> (32-s);

    result.trim();
    
    return result;
}

BigInt& BigInt::operator<<=(const uint32_t m)
{
    return (*this = *this << m);
}

BigInt operator%(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid() && b > 0);
    if(a < b)
        return a;
    if(a == b)
        return 0;
    return a - (a/b)*b;
}

BigInt& BigInt::operator%=(const BigInt &a)
{
    return (*this = *this % a);
}

bool operator==(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());

    return a.sign == b.sign && equals(a, b);
}

bool operator!=(const BigInt &a, const BigInt &b)
{
    return ! (a==b);
}

bool operator<(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());
    
    if(a.sign == NEG && b.sign == NEG)
        return !lesser(a, b);
    else if(a.sign == NEG && b.sign == POS)
        return true;
    else if(a.sign == POS && b.sign == NEG)
        return false;
    else
        return lesser(a, b);
}

bool operator<=(const BigInt &a, const BigInt &b)
{
    return !(a > b);
}

bool operator>(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());

    if(a.sign == NEG && b.sign == NEG)
        return !greater(a, b);
    else if(a.sign == NEG && b.sign == POS)
        return false;
    else if(a.sign == POS && b.sign == NEG)
        return true;
    else
        return greater(a, b);
}

bool operator>=(const BigInt &a, const BigInt &b)
{
    return !(a < b);
}

bool operator!(const BigInt &a)
{
    assert(a.isValid());

    if(a.size != 1)
        return false;
    return a.bits[0] == 0;
}
 
 
BigInt operator&(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());

    BigInt result;

    result.size = std::min(a.size, b.size);
    uint32_t l = num(result.size);
    result.bits = new uint32_t[l];
    memset(result.bits, 0, l*sizeof(uint32_t));
    for(uint32_t i = 0; i < l; ++i)
        result.bits[i] = a.bits[i] & b.bits[i];

    result.trim();
    
    return result;
}

BigInt& BigInt::operator&=(const BigInt &a)
{
    return (*this = *this & a);
}

BigInt operator|(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());

    BigInt result;

    uint32_t na = num(a.size);
    uint32_t nb = num(b.size);
    uint32_t l = std::max(na,nb);
    result.size = std::max(a.size, b.size);
    result.bits = new uint32_t[num(result.size)];
    memset(result.bits, 0, num(result.size)*sizeof(uint32_t));
    
    for(uint32_t i = 0; i < l; ++i)
    {
        if(i < na && i < nb)
            result.bits[i] = a.bits[i] | b.bits[i];
        else if(i < na)
            result.bits[i] = a.bits[i];
        else
            result.bits[i] = b.bits[i];
    }
    
    result.trim();
    
    return result;
}

BigInt& BigInt::operator|=(const BigInt &a)
{
    return (*this = *this | a);
}

BigInt operator^(const BigInt &a, const BigInt &b)
{
    assert(a.isValid() && b.isValid());

    BigInt result;

    uint32_t na = num(a.size);
    uint32_t nb = num(b.size);
    result.size = std::max(a.size, b.size);
    uint32_t l = num(result.size);
    result.bits = new uint32_t[l];
    memset(result.bits, 0, l*sizeof(uint32_t));
    for(uint32_t i = 0; i < l; ++i)
    {
        if(i < na && i < nb)
            result.bits[i] = a.bits[i] ^ b.bits[i];
        else if(i < na)
            result.bits[i] = a.bits[i];
        else
            result.bits[i] = b.bits[i];
    }
    
    result.trim();
    
    return result;
}

BigInt& BigInt::operator^=(const BigInt &a)      
{
    return (*this = *this ^ a);
}

// Perform one step : (a * b) / r mod m
BigInt montgomeryStep(const BigInt &a, const BigInt &b, const BigInt &m, uint32_t r)
{
    BigInt result = 0;
    uint32_t j = 0;
    while(r > 0)
    {
        if(a.bits[j/32] & BITS[j%32])
            result.fastAdd(b);
        
        if(result.bits[0] & BITS[0])
            result.fastAdd(m);
     
        ++j; 
        --r;
        result.fastShr();
    }
    
    if(result >= m)
        return result - m;
    
    return result;
}
  
// Implementation using Montgomery algorithm
BigInt modPow(const BigInt &a, const BigInt &expn, const BigInt &modulus)
{
    assert(a.isValid() && expn.isValid() && modulus.isValid() && modulus != 0);

    if(modulus == 1)
        return 0;    
    if(expn == 0)
        return 1;
    if(a == 1)
        return 1;
    if(expn == 1)
       return a % modulus;
    
    const uint32_t r = 8*modulus.size;
    
    // convert a in montgomery world
    BigInt montA = (a << r) % modulus;
    
    BigInt tmp;
    const uint32_t n = expn.numBits();
    uint32_t j = 0; 
    while(j <= n)
    {
        
        if(expn.bits[j/32] & BITS[j%32])
        {
            if(tmp.isValid())
                tmp = montgomeryStep(tmp, montA, modulus, r);
            else
                tmp = montA;
        }
        ++j;
        if(j <= n)  
            montA = montgomeryStep(montA, montA, modulus, r);
        
    }
    
    // convert tmp to normal world
    return montgomeryStep(tmp, 1, modulus, r);
}

// Implementation as described in FIPS.186-4, Appendix C.1
BigInt invMod(const BigInt &a, const BigInt &modulus)
{
    assert(a.isValid() && modulus.isValid() && 0 < a && a < modulus);
    
    BigInt i = modulus;
    BigInt j = a;
    BigInt y2 = 0;
    BigInt y1 = 1;
    do
    {
        BigInt quotient = i / j;
        BigInt remainder = i - (j * quotient);
        BigInt y = y2 - (y1 * quotient);
        i = j; 
        j = remainder; 
        y2 = y1;
        y1 = y;
    }while(j > 0);
    

    assert(i == 1);
    
    y2 %= modulus;
    if(y2 < 0)
        y2 += modulus;
        
    return y2;
}

bool BigInt::isValid() const
{
    return size != 0 && bits != 0;
}

void BigInt::print() const
{
    assert(isValid());
    
    printf("size: %lu bytes\n", size);
    uint32_t n = num(size);
    if(sign == NEG)
        printf("- ");
    for(int i = n-1; i >= 0; --i)
        printf("%08x ", (int)bits[i]);
    printf("\n");
}

// return a + b
BigInt add(const BigInt &a, const BigInt &b)
{
    BigInt result;
        
    result.size = std::max(a.size, b.size) + 1;
    size_t l = num(result.size);
    result.bits = new uint32_t[l];
    memset(result.bits, 0, sizeof(uint32_t)*l);
    uint32_t al = num(a.size);
    uint32_t bl = num(b.size);
    uint32_t carry = 0;
    for(int i = 0; i < (int)l; ++i)
    {
        uint32_t tmpA = 0, tmpB = 0;
        if(i < (int)al)
            tmpA = a.bits[i];
        if(i < (int)bl)
            tmpB = b.bits[i];
        result.bits[i] = tmpA + tmpB + carry;
        carry = result.bits[i] < std::max(tmpA, tmpB);
    }

    result.trim();
    result.checkZero();
    
    return result;
}

// return a - b
// Assume that a > b
BigInt sub(const BigInt &a, const BigInt &b)
{
    BigInt result;
    result.size = a.size;
    uint32_t l = num(a.size);
    result.bits = new uint32_t[l];
    memset(result.bits, 0, sizeof(uint32_t)*l);
    uint32_t bl = num(b.size);
    uint8_t borrow = 0;
    for(uint32_t i = 0; i < l; ++i)
    {
        uint32_t tmpA = a.bits[i], tmpB = 0;
        if(i < bl)
            tmpB = b.bits[i];
            
        if(borrow)  
        {
            if(tmpA == 0)
                tmpA = 0xFFFFFFFF;
            else
            {
                --tmpA;
                borrow = 0;
            }
        }
        if(tmpA >= tmpB)
            result.bits[i] = tmpA - tmpB;
        else 
        {
            result.bits[i] = 0xFFFFFFFF - tmpB;
            result.bits[i] += tmpA + 1;
            borrow = 1;
        }
    }
    result.trim();
    result.checkZero();
    
    return result;  
}

BigInt mul(const BigInt &a, const BigInt &b)
{
    BigInt result;          
    result.size = a.size + b.size;
    result.bits = new uint32_t[num(result.size)];
    memset(result.bits, 0, sizeof(uint32_t)*num(result.size));
    for(int i = 0; i < (int)num(a.size); ++i)
    {
        uint64_t carry = 0;
        for(int j = 0; j < (int)num(b.size); ++j)
        {
            uint64_t tmp = (uint64_t)a.bits[i] * (uint64_t)b.bits[j] + carry;        
            uint32_t t = result.bits[i+j];
            result.bits[i+j] += tmp;
            carry = tmp >> 32;     
            if(t > result.bits[i+j])
                ++carry;                    
        }
        if(carry != 0)
            result.bits[i+num(b.size)] += carry;
    }
    
    result.trim();

    return result;
}

BigInt div(const BigInt &a, const BigInt &b)
{
    BigInt u = a; 
    const uint32_t m = a.numBits() - b.numBits();
    BigInt q;
    q.size = m/8 + 1;
    q.bits = new uint32_t[num(q.size)];
    memset(q.bits, 0, num(q.size)*sizeof(uint32_t));
    BigInt tmp = b;
    tmp <<= m;
    for(int j = m; j >= 0; --j)
    {
        if(tmp <= u)
        {   
            u -= tmp;
            q.bits[j/32] |= BITS[j%32]; 
        }
        tmp >>= 1;
    }
    q.trim();

    return q;
}

bool equals(const BigInt &a, const BigInt &b)
{
    if(a.size != b.size)
        return false;
        
    uint32_t l = num(a.size);
    for(int i = 0; i < (int)l; ++i)
        if(a.bits[i] != b.bits[i])
            return false;
    return true;
}

bool lesser(const BigInt &a, const BigInt &b)
{
    if(a.size < b.size)
        return true;
    if(a.size > b.size)
        return false;
        
    uint32_t l = num(a.size);
    for(int i = l-1; i >= 0; --i)
    {
        if(a.bits[i] < b.bits[i])
            return true;
        else if(a.bits[i] > b.bits[i])
            return false;
    }
    return false;
}
        
bool greater(const BigInt &a, const BigInt &b)
{
    if(a.size > b.size)
        return true;
    if(a.size < b.size)
        return false;
    uint32_t l = num(a.size);
    for(int i = l-1; i >= 0; --i)
    {
        if(a.bits[i] > b.bits[i])
            return true;
        else if(a.bits[i] < b.bits[i])
            return false;       
    }
    return false;
}


void BigInt::fastAdd(const BigInt &b)
{
    uint32_t al = num(size);
    uint32_t bl = num(b.size);
    uint32_t rsize = std::max(size, b.size) + 1;
    size_t l = num(rsize);
    
    if(l > al)
    {
        bits = (uint32_t*)realloc(bits, sizeof(uint32_t)*l);
        memset(&bits[al], 0, (l-al)*sizeof(uint32_t));
        size = rsize;
    }

    uint32_t carry = 0;
    for(int i = 0; i < (int)l; ++i)
    {
        uint32_t tmpA = 0, tmpB = 0;
        if(i < (int)al)
            tmpA = bits[i];
        if(i < (int)bl)
            tmpB = b.bits[i];
        bits[i] = tmpA + tmpB + carry;
        carry = bits[i] < std::max(tmpA, tmpB);
    }

    trim();
}

void BigInt::fastShr()
{
    uint32_t lastBit = 0;
    uint32_t tmp;
    for(int i = num(size)-1; i >= 0; --i)
    {
        tmp = bits[i] & BITS[0];
        bits[i] >>= 1;
        bits[i] |= (lastBit ? BITS[31] : 0);
        lastBit = tmp;
    }

    trim();
}

void BigInt::trim()
{
    assert(isValid());

    
    uint8_t *tmp = (uint8_t*)bits;
    uint32_t newSize = size;
    while(newSize > 0 && tmp[newSize-1] == 0)
        newSize--;
    if(newSize == 0)
        newSize = 1;
    uint32_t n = num(newSize);
    if(n < num(size))
    {
        bits = (uint32_t*)realloc(bits, n*sizeof(uint32_t));
        if(newSize % 4 != 0)
            bits[n-1] &= BITS2[newSize%4];
    }
    size = newSize; 
}

uint32_t BigInt::numBits() const
{
    assert(isValid());

    uint32_t n = (size-1)*8;
    uint8_t a = bits[num(size)-1] >> ((size-1)%4)*8;
    uint8_t tmp2 = 8;
    while(!(a & 0x80)) 
    {
        a <<= 1;
        --tmp2;
    }
    n += tmp2;

    return n;
}

// Ensure that there is no negative zero
void BigInt::checkZero()
{
    assert(isValid());
    
    if(size == 1 && bits[0] == 0)
        sign = POS;
}
