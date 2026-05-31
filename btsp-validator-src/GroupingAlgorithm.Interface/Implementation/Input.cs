using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    public class Input : IInput
    {
        /// <summary>
        /// The order types.
        /// </summary>
        [JsonPropertyName("OrderTypes")]
        public List<OrderType> JsonOrderTypes { get; }

        /// <summary>
        /// The order types.
        /// </summary
        [JsonIgnore]
        public IList<IOrderType> OrderTypes { get; }

        /// <summary>
        /// The orders.
        /// </summary>
        [JsonPropertyName("Orders")]
        public List<Order> JsonOrders { get; }

        /// <summary>
        /// The orders.
        /// </summary>
        [JsonIgnore]
        public IList<IOrder> Orders { get; }

        /// <summary>
        /// The resources.
        /// </summary>
        [JsonPropertyName("Resources")]
        public List<Resource> JsonResources { get; }

        /// <summary>
        /// The resources.
        /// </summary>
        [JsonIgnore]
        public IList<IResource> Resources { get; }

        /// <summary>
        /// The objectives used for optimization.
        /// </summary>
        [JsonPropertyName("Objectives")]
        public List<Objective> JsonObjectives { get; }

        /// <summary>
        /// The objectives used for optimization.
        /// </summary>
        [JsonIgnore]
        public IList<IObjective> Objectives { get; }

        /// <summary>
        /// The mode used for the ranking of planning groups by order priorities.
        /// </summary>
        public RankingMode RankingMode { get; }

        /// <summary>
        /// Algorithm parameters given as part of the input
        /// </summary>
        [JsonIgnore]
        public Parameters Parameters { get; set; }

        /// <summary>
        /// An additional raw form of the parameters, used by JSON de-/serialization
        /// </summary>
        [JsonPropertyName("Parameters")]
        public Parameters JsonParameters { get; }

        /// <summary>
        /// Create a new input.
        /// </summary>
        /// <param name="orderTypes">The order types.</param>
        /// <param name="orders">The orders.</param>
        /// <param name="resources">The resources.</param>
        /// <param name="objectives">The objectives used for optimization.</param>
        /// <param name="rankingMode">The mode used for the ranking of planning groups by order priorities.</param>
        public Input(
            IList<IOrderType> orderTypes,
            IList<IOrder> orders,
            IList<IResource> resources,
            IList<IObjective> objectives,
            RankingMode rankingMode,
            Parameters parameters
        )
        {
            OrderTypes = orderTypes;
            JsonOrderTypes = orderTypes.Select(x => new OrderType(x.Id, x.TargetGroupSize, x.MinGroupSize, x.MaxGroupSize)).ToList();

            Orders = orders;
            JsonOrders = orders.Select(x => new Order(x.Id, x.TypeId, x.Quantity, x.Priority, x.ValidResourceIds)).ToList();

            Resources = resources;
            JsonResources = resources.Select(x => new Resource(x.Id)).ToList();

            Objectives = objectives;
            JsonObjectives = objectives.Select(x => new Objective(x.Type, x.Priority, x.Weight)).ToList();

            RankingMode = rankingMode;

            Parameters = parameters;
            JsonParameters = new Parameters(parameters.MaxRunTimeMS);

            ValidateInput(this);
        }

        /// <summary>
        /// Create an input.
        /// </summary>
        /// <param name="other">Another input that should be copied</param>
        public Input(IInput other) : this(other.OrderTypes, other.Orders, other.Resources, other.Objectives, other.RankingMode, other.Parameters)
        {

        }

        /// <summary>
        /// Create a new input.
        /// </summary>
        /// <param name="jsonOrderTypes">The order types.</param>
        /// <param name="jsonOrders">The orders.</param>
        /// <param name="jsonResources">The resources.</param>
        /// <param name="jsonObjectives">The objectives used for optimization.</param>
        /// <param name="rankingMode">The mode used for the ranking of planning groups by order priorities.</param>
        [JsonConstructor]
        public Input(
            List<OrderType> jsonOrderTypes,
            List<Order> jsonOrders,
            List<Resource> jsonResources,
            List<Objective> jsonObjectives,
            RankingMode rankingMode,
            Parameters jsonParameters
        )
        {
            OrderTypes = jsonOrderTypes.Select(x => (IOrderType)x).ToList();
            JsonOrderTypes = jsonOrderTypes;

            Orders = jsonOrders.Select(x => (IOrder)x).ToList();
            JsonOrders = jsonOrders;

            Resources = jsonResources.Select(x => (IResource)x).ToList();
            JsonResources = jsonResources;

            Objectives = jsonObjectives.Select(x => (IObjective)x).ToList();
            JsonObjectives = jsonObjectives;

            RankingMode = rankingMode;

            Parameters = jsonParameters;
            JsonParameters = jsonParameters;

            ValidateInput(this);
        }

        /// <summary>
        /// Validates the given input.
        /// </summary>
        /// <param name="input">The input that should be validated.</param>
        public static void ValidateInput(IInput input)
        {
            if (!input.OrderTypes.Any())
            {
                throw new ArgumentException($"{nameof(input)} contains an empty order type list.", nameof(input));
            }

            if (!input.Orders.Any())
            {
                throw new ArgumentException($"{nameof(input)} contains an empty order list.", nameof(input));
            }

            if (!input.Resources.Any())
            {
                throw new ArgumentException($"{nameof(input)} contains an empty resource list.", nameof(input));
            }

            foreach (IOrderType orderType in input.OrderTypes)
            {
                if (input.OrderTypes.Count(x => x.Id == orderType.Id) > 1)
                {
                    throw new ArgumentException(
                        $"{nameof(input)} contains more than one order type with ID {orderType.Id}",
                        nameof(input)
                    );
                }
            }

            foreach (IOrder order in input.Orders)
            {
                if (input.Orders.Count(x => x.Id == order.Id) > 1)
                {
                    throw new ArgumentException(
                        $"{nameof(input)} contains more than one order with ID {order.Id}",
                        nameof(input)
                    );
                }

                if (!input.OrderTypes.Any(x => x.Id == order.TypeId))
                {
                    throw new ArgumentException(
                        $"Order type with ID {order.TypeId} does not exist but is set as type for order with ID {order.Id}.",
                        nameof(input)
                        );
                }

                foreach (int validResourceId in order.ValidResourceIds)
                {
                    if (!input.Resources.Any(x => x.Id == validResourceId))
                    {
                        throw new ArgumentException(
                            $"Resource with ID {validResourceId} does not exist but is contained in list of valid resources for order with ID {order.Id}.",
                            nameof(input)
                            );
                    }
                }
            }

            foreach (IResource resource in input.Resources)
            {
                if (input.Resources.Count(x => x.Id == resource.Id) > 1)
                {
                    throw new ArgumentException(
                        $"{nameof(input)} contains more than one resource with ID {resource.Id}.",
                        nameof(input)
                    );
                }
            }

            foreach (IObjective objective in input.Objectives)
            {
                if (input.Objectives.Count(x => x.Type == objective.Type) > 1)
                {
                    throw new ArgumentException(
                        $"{nameof(input)} contains more than one objective of type {objective.Type}.",
                        nameof(input)
                    );
                }
            }

        }

        public IEnumerable<IInput> PartitionInput(bool partitionRuntime)
        {
            IList<IInput> partitionedInputs = new List<IInput>();

            double runtimePerOrder = Parameters.MaxRunTimeMS / Orders.Count;

            foreach (IOrderType orderType in OrderTypes)
            {
                IList<IOrder> orders = Orders.Where(order => order.TypeId == orderType.Id).ToList();

                long runtime = Parameters.MaxRunTimeMS;

                if (partitionRuntime)
                {
                    runtime = (long)Math.Round(runtimePerOrder * orders.Count);
                }

                IInput input = new Input(new List<IOrderType> { orderType }, orders, Resources, Objectives, RankingMode, new Parameters(runtime));
                partitionedInputs.Add(input);
            }

            return partitionedInputs;
        }
    }
}
