using System;
using System.Text.Json.Serialization;

namespace GroupingAlgorithm.Interface.Implementation
{
    /// <summary>
    /// A resource.
    /// </summary>
    public class Resource : IResource
    {
        /// <summary>
        /// The ID of the resource.
        /// </summary>
        [JsonPropertyName("ResourceId")]
        public int Id { get; }

        /// <summary>
        /// Create a resource.
        /// </summary>
        /// <param name="id">The ID of the resource.</param>
        public Resource(int id)
        {
            if (id <= 0)
            {
                throw new ArgumentOutOfRangeException(nameof(id), $"{nameof(id)} must be > 0.");
            }

            Id = id;
        }
    }
}
