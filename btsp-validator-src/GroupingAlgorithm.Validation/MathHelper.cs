using System.Collections.Generic;
using System.Linq;

namespace GroupingAlgorithm.Validation
{
    public static class MathHelper
    {
        // Functions for least common multiple and greatest common divisor
        public static long Lcm(IEnumerable<long> values) => values.Aggregate(Lcm);

        public static long Lcm(long firstValue, long secondValue) =>
            firstValue == 0 ? 0 : firstValue / Gcd(firstValue, secondValue) * secondValue;

        public static long Gcd(IEnumerable<long> values) => values.Aggregate(Gcd);

        public static long Gcd(long firstValue, long secondValue)
        {
            if (firstValue < 0)
                firstValue = -firstValue;
            if (secondValue < 0)
                secondValue = -secondValue;
            while (firstValue != 0 && secondValue != 0)
            {
                if (firstValue > secondValue)
                    firstValue %= secondValue;
                else
                    secondValue %= firstValue;
            }
            return firstValue | secondValue;
        }
    }
}