#ifndef BIGINT_H
#define BIGINT_H

#include <stdint.h>

#define POS (true)
#define NEG (false)
    
class BigInt
{
    public :
    
        BigInt();
        BigInt(int32_t a);
        BigInt(const BigInt &a);
        BigInt& operator=(const BigInt& a);
        virtual ~BigInt();
        
        void importData(uint8_t *data, uint32_t length, bool sign = POS);
        void exportData(uint8_t *data, uint32_t length, bool &sign);
        
        // Arithmetic operations
        friend BigInt operator+(const BigInt &a, const BigInt &b);
        BigInt& operator+=(const BigInt &b);
        BigInt& operator++();
        BigInt operator++(int);
        
        friend BigInt operator-(const BigInt &a, const BigInt &b);
        friend BigInt operator-(const BigInt &a);
        BigInt& operator-=(const BigInt &b);
        BigInt& operator--();
        BigInt operator--(int);

        friend BigInt operator*(const BigInt &a, const BigInt &b);
        BigInt& operator*=(const BigInt &b);

        friend BigInt operator/(const BigInt &a, const BigInt &b);
        BigInt& operator/=(const BigInt &b);
                
        friend BigInt operator>>(const BigInt &a, const uint32_t m);
        BigInt& operator>>=(const uint32_t m);
        friend BigInt operator<<(const BigInt &a, const uint32_t m);
        BigInt& operator<<=(const uint32_t m);
        
        friend BigInt operator%(const BigInt &a, const BigInt &b);
        BigInt& operator%=(const BigInt &a);
        
        // Boolean operations
        friend bool operator==(const BigInt &a, const BigInt &b);
        friend bool operator!=(const BigInt &a, const BigInt &b); 
        friend bool operator<(const BigInt &a, const BigInt &b);
        friend bool operator<=(const BigInt &a, const BigInt &b);
        friend bool operator>(const BigInt &a, const BigInt &b);
        friend bool operator>=(const BigInt &a, const BigInt &b);
        friend bool operator!(const BigInt &a);
        
        // Bits operations 
        friend BigInt operator&(const BigInt &a, const BigInt &b);
        BigInt& operator&=(const BigInt &a);
        friend BigInt operator|(const BigInt &a, const BigInt &b);
        BigInt& operator|=(const BigInt &a);
        friend BigInt operator^(const BigInt &a, const BigInt &b);
        BigInt& operator^=(const BigInt &a);
        
        // modular exponentiation
        friend BigInt modPow(const BigInt &a, const BigInt &expn, const BigInt &modulus);
        
        // invert modular
        friend BigInt invMod(const BigInt &a, const BigInt &modulus);
        
        // miscellaneous
        bool isValid() const;
        uint32_t numBits() const;    
        
        // debug
        void print() const;

    private :
    
        friend BigInt add(const BigInt &a, const BigInt &b);
        friend BigInt sub(const BigInt &a, const BigInt &b);
        friend BigInt mul(const BigInt &a, const BigInt &b);
        friend BigInt div(const BigInt &a, const BigInt &b);
        
        friend bool equals(const BigInt &a, const BigInt &b);
        friend bool lesser(const BigInt &a, const BigInt &b);
        friend bool greater(const BigInt &a, const BigInt &b);

        // fast operations
        void fastAdd(const BigInt &b);
        void fastShr();
        
        void trim();  
        void checkZero();
     
        friend BigInt montgomeryStep(const BigInt &a, const BigInt &b, const BigInt &m, uint32_t r);
        friend BigInt montgomeryStep2(const BigInt &a, const BigInt &m, uint32_t r);
        
        bool sign;
        uint32_t size;  // smaller number of bytes needed to represent integer
        uint32_t *bits;
};


#endif