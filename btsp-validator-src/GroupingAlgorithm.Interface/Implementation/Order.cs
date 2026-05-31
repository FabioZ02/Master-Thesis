using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// An order.
    /// </summary>
    public class Order : IOrder
    {
        /// <summary>
        /// The ID of the order.
        /// </summary>
        [JsonPropertyName("OrderId")]
        public int Id { get; set; }

        /// <summary>
        /// The ID of the order type.
        /// </summary>
        public int TypeId { get; }

        /// <summary>
        /// The quantity of the order.
        /// </summary>
        public int Quantity { get; }

        /// <summary>
        /// The priority of the order, where 1 is the most important one.
        /// </summary>
        public int Priority { get; set; }

        /// <summary>
        /// The IDs of the resources on which the order can be planned.
        /// </summary>
        public List<int> ValidResourceIds { get; }

        /// <summary>
        /// Create a new order.
        /// </summary>
        /// <param name="id">The ID of the order.</param>
        /// <param name="typeId">The ID of the order type.</param>
        /// <param name="quantity">The quantity of the order.</param>
        /// <param name="priority">The priority of the order, where 1 is the most important one.</param>
        /// <param name="validResourceIds">The IDs of the resources on which the order can be planned.</param>
        public Order(int id, int typeId, int quantity, int priority, List<int> validResourceIds)
        {
            if (id <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(id), $"{nameof(id)} must be > 0.");
            }

            if (quantity <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(quantity), $"{nameof(quantity)} must be > 0.");
            }

            if (priority <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(priority), $"{nameof(priority)} must be > 0.");
            }

            if (validResourceIds == null)
            {
                throw new ArgumentNullException(nameof(validResourceIds));
            }

            if (!validResourceIds.Any())
            {
                throw new ArgumentException(nameof(validResourceIds), $"{nameof(validResourceIds)} must not be empty.");
            }

            Id = id;
            TypeId = typeId;
            Quantity = quantity;
            Priority = priority;
            ValidResourceIds = validResourceIds;
        }
    }
}
