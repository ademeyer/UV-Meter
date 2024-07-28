#include <math.h>
#include <stdlib.h>
struct number
{
    uint8_t value;
    int8_t power;
    number() : value(0), power(0) {}
    number(int v, int p) : value(v), power(p) {}
};

class FloatingInteger 
{
public:
    number FloatToNumber(double num, int maxP)
    {
        int Power = 0;
        num *= pow(10, maxP);
        Power -= maxP;

        while(num > 255.00 && abs(Power) < 128)
        {
            num /= 10;
            ++Power;
        }

        while (round(num) < 1.00 && round(num * 10) < 255.00 && abs(Power) < 128)
        {
            num *= 10;
            --Power;
        }
        return number(round(num), Power);
    }
    float NumberToFloat(number num)
    {
        return num.value * pow(10, num.power);
    }
};