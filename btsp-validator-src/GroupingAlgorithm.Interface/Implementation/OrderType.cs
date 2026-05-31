using System;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// The type of an order.
    /// </summary>
    public class OrderType : IOrderType
    {
        /// <summary>
        /// The ID of the order type.
        /// </summary>
        [JsonPropertyName("OrderTypeId")]
        public int Id { get; }

        /// <summary>
        /// The targeted group size.
        /// </summary>
        public int TargetGroupSize { get; }

        /// <summary>
        /// The minimal group size.
        /// </summary>
        public int MinGroupSize { get; }

        /// <summary>
        /// The maximal group size.
        /// </summary>
        public int MaxGroupSize { get; set; }

        /// <summary>
        /// Create a new order type.
        /// </summary>
        /// <param name="id">The ID of the order type.</param>
        /// <param name="targetGroupSize">The targeted group size.</param>
        /// <param name="minGroupSize">The minimal group size.</param>
        /// <param name="maxGroupSize">The maximal group size.</param>
        public OrderType(int id, int targetGroupSize, int minGroupSize, int maxGroupSize)
        {
            if (id <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(id), $"{nameof(id)} must be > 0.");
            }

            if (targetGroupSize <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(targetGroupSize), $"{nameof(targetGroupSize)} must be > 0.");
            }

            if (maxGroupSize < targetGroupSize)
            {
                throw new ArgumentOutOfRangeException(nameof(maxGroupSize), $"{nameof(maxGroupSize)} must be >= {nameof(targetGroupSize)}.");
            }

            if (minGroupSize < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(minGroupSize), $"{nameof(minGroupSize)} must be >= 0.");
            }

            if (minGroupSize > targetGroupSize)
            {
                throw new ArgumentOutOfRangeException(nameof(minGroupSize), $"{nameof(minGroupSize)} must be <= {nameof(targetGroupSize)}.");
            }

            Id = id;
            TargetGroupSize = targetGroupSize;
            MinGroupSize = minGroupSize;
            MaxGroupSize = maxGroupSize;
        }
    }
}
