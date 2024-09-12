#include <bare.h>
#include <js.h>
#include <annoylib.h>
#include <kissrandom.h>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

// Define the different Annoy index types
using AnnoyIndexEuclidean = Annoy::AnnoyIndex<int, float, Annoy::Euclidean, Annoy::Kiss32Random, Annoy::AnnoyIndexSingleThreadedBuildPolicy>;
using AnnoyIndexAngular = Annoy::AnnoyIndex<int, float, Annoy::Angular, Annoy::Kiss32Random, Annoy::AnnoyIndexSingleThreadedBuildPolicy>;

// Annoy Index Context using std::variant to store different types of AnnoyIndex
struct AnnoyContext {
    std::variant<AnnoyIndexEuclidean*, AnnoyIndexAngular*> index;
};

// Helper function to convert JS strings to C++ strings
std::string get_cpp_string(js_env_t *env, js_value_t* js_str) {
    size_t len = 0;
    utf8_t *str = nullptr;
    std::string res;

    js_get_value_string_utf8(env, js_str, nullptr, 0, &len);
    str = new utf8_t[len + 1];
    str[len] = '\0';
    js_get_value_string_utf8(env, js_str, str, len, nullptr);

    res = reinterpret_cast<char *>(str);
    delete[] str;
    return res;
}

// Helper function to retrieve arguments from JavaScript
static bool get_js_arguments(js_env_t *env, js_callback_info_t *info, size_t *argc, js_value_t ***argv_js) {
    int err = js_get_callback_info(env, info, argc, nullptr, nullptr, nullptr);
    if (err != 0) {
        return false;
    }

    *argv_js = new js_value_t *[*argc];
    err = js_get_callback_info(env, info, argc, *argv_js, nullptr, nullptr);
    return err == 0;
}

// Helper function to throw a JavaScript error
static void throw_js_error(js_env_t *env, const char *error_type, const char *message) {
    js_throw_error(env, error_type, message);
}

// Create a new Annoy Index
js_value_t *annoy_create_index(js_env_t *env, js_callback_info_t *info) {
    size_t argc;
    js_value_t **argv;
    get_js_arguments(env, info, &argc, &argv);

    if (argc < 2) {
        throw_js_error(env, "ARGUMENT_ERROR", "Expected 2 arguments: dimension and metric.");
        return nullptr;
    }

    int32_t dimension;
    js_get_value_int32(env, argv[0], &dimension);
    std::string metric = get_cpp_string(env, argv[1]);

    AnnoyContext *ctx = new AnnoyContext();

    if (metric == "euclidean") {
        ctx->index = new AnnoyIndexEuclidean(dimension);
    } else if (metric == "angular") {
        ctx->index = new AnnoyIndexAngular(dimension);
    } else {
        throw_js_error(env, "ARGUMENT_ERROR", "Invalid metric.");
        delete ctx;
        return nullptr;
    }

    js_value_t *result = nullptr;
    js_create_external(env, ctx, [](js_env_t *env, void *data, void *finalize_hint) {
        AnnoyContext *ctx = reinterpret_cast<AnnoyContext*>(data);
        std::visit([](auto* index) { delete index; }, ctx->index); // Clean up based on the variant type
        delete ctx;
    }, nullptr, &result);

    return result;
}

// Add items to the Annoy Index
js_value_t *annoy_add_item(js_env_t *env, js_callback_info_t *info) {
    size_t argc;
    js_value_t **argv;
    get_js_arguments(env, info, &argc, &argv);

    AnnoyContext *ctx = nullptr;
    js_get_value_external(env, argv[0], (void **)&ctx);
    
    int32_t item;
    js_get_value_int32(env, argv[1], &item);

    std::vector<float> vector;
    js_get_arraybuffer_info(env, argv[2], reinterpret_cast<void**>(&vector), nullptr);

    // Visit the variant to call the correct type's add_item method
    std::visit([&](auto* index) { index->add_item(item, vector.data()); }, ctx->index);

    js_value_t *result = nullptr;
    js_get_null(env, &result);
    return result;
}

// Build the Annoy Index
js_value_t *annoy_build_index(js_env_t *env, js_callback_info_t *info) {
    size_t argc;
    js_value_t **argv;
    get_js_arguments(env, info, &argc, &argv);

    AnnoyContext *ctx = nullptr;
    js_get_value_external(env, argv[0], (void **)&ctx);

    int32_t num_trees;
    js_get_value_int32(env, argv[1], &num_trees);

    // Visit the variant to call the correct type's build method
    std::visit([&](auto* index) { index->build(num_trees); }, ctx->index);

    js_value_t *result = nullptr;
    js_get_null(env, &result);
    return result;
}

// Search for nearest neighbors
js_value_t *annoy_get_nns_by_vector(js_env_t *env, js_callback_info_t *info) {
    size_t argc;
    js_value_t **argv;
    get_js_arguments(env, info, &argc, &argv);

    AnnoyContext *ctx = nullptr;
    js_get_value_external(env, argv[0], (void **)&ctx);

    std::vector<float> query_vector;
    int32_t num_neighbors;
    js_get_value_int32(env, argv[2], &num_neighbors);
    
    std::vector<int> neighbors;
    std::vector<float> distances;

    js_get_arraybuffer_info(env, argv[1], reinterpret_cast<void**>(&query_vector), nullptr);

    // Visit the variant to call the correct type's get_nns_by_vector method
    std::visit([&](auto* index) {
        index->get_nns_by_vector(query_vector.data(), num_neighbors, -1, &neighbors, &distances);
    }, ctx->index);

    js_value_t *result = nullptr;
    js_create_object(env, &result);

    js_value_t *neighbors_js;
    js_create_arraybuffer(env, neighbors.size() * sizeof(int), (void**)neighbors.data(), &neighbors_js);
    js_set_named_property(env, result, "neighbors", neighbors_js);

    js_value_t *distances_js;
    js_create_arraybuffer(env, distances.size() * sizeof(float), (void**)distances.data(), &distances_js);
    js_set_named_property(env, result, "distances", distances_js);

    return result;
}

// Expose the Annoy functions to Bare
static js_value_t *init(js_env_t *env, js_value_t *exports) {
    int err = 0;

#define V(name, fn) \
    { \
        js_value_t *val; \
        err = js_create_function(env, name, -1, fn, NULL, &val); \
        assert(err == 0); \
        err = js_set_named_property(env, exports, name, val); \
        assert(err == 0); \
    }

    V("annoyCreateIndex", annoy_create_index)
    V("annoyAddItem", annoy_add_item)
    V("annoyBuildIndex", annoy_build_index)
    V("annoyGetNnsByVector", annoy_get_nns_by_vector)

#undef V
    return exports;
}

BARE_MODULE(annoy_module, init)