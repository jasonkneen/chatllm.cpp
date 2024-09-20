#pragma once

#include <vector>
#include <memory>

#include "ggml-backend.h"

namespace chatllm
{
    namespace ggml
    {
        typedef ggml_tensor tensor;
        typedef ggml_type   type;

        tensor *init_tensor(ggml::tensor  *tensor,
                            ggml::type     type,
                            int            n_dims,
                            const int64_t *ne);

        size_t element_size(const ggml::tensor *tensor);
        size_t nbytes(const ggml::tensor *tensor);

        int n_dims(const ggml::tensor * tensor);

        void set_name(tensor *tensor, const char *name);

        typedef bool (* need_observe_tensor_evaluation_callback)(ggml::tensor *tensor, void *user_data);
        typedef bool (* observe_tensor_evaluation_callback)(ggml::tensor *tensor, void *user_data);
    }

    // Is `ggml_backend_buffer_type_t` a good name?
    typedef ggml_backend_buffer_type_t  ggml_backend_allocator;

    class LayerBufAllocator;

    class BackendBuffer
    {
    public:
        friend LayerBufAllocator;

        void *get_base(void);

        size_t get_size(void) const;

        bool is_host(void);

        ~BackendBuffer();

        void assign_to(ggml::tensor *tensor, size_t offset = 0);

    protected:
        BackendBuffer(ggml_backend_buffer_t buf);

        ggml_backend_buffer_t buf;
    };

    class Backend;

    class BackendBufAllocator
    {
    public:
        enum Usage
        {
            Matrix,
            Others,
        };

        virtual BackendBuffer *alloc(size_t size, Usage usage = Usage::Others) = 0;
        virtual bool alloc(ggml::tensor *tensor) = 0;

        virtual size_t get_alignment(Usage usage) const = 0;
        virtual size_t get_max_size(Usage usage) const = 0;

        virtual bool supported_by_backend(Backend *backend, ggml::tensor *tensor)
        {
            return false;
        }

    protected:
        size_t total = 0;
    };

    class LayerBufAllocator : public BackendBufAllocator
    {
    public:
        friend class BackendContext;

        LayerBufAllocator();
        LayerBufAllocator(ggml_backend_allocator alloc, Backend *backend);
        LayerBufAllocator(ggml_backend_allocator alloc_matrix, ggml_backend_allocator alloc_others, Backend *backend);

        BackendBuffer *alloc(size_t size, Usage usage = Usage::Others) override;
        bool alloc(ggml::tensor *tensor) override;

        bool supported_by_backend(Backend *backend, ggml::tensor *tensor) override;

        size_t get_alignment(Usage usage) const override;

        size_t get_max_size(Usage usage) const override;

        void free_all_buffers(void);

        Backend *get_backend(void);

        bool operator ==(const LayerBufAllocator &b);

    protected:
        Usage detect_usage(ggml::tensor *tensor);
        ggml_backend_allocator get_allocator(Usage usage);
        ggml_backend_allocator get_allocator(ggml::tensor *tensor);

    protected:
        ggml_backend_allocator alloc_matrix;
        ggml_backend_allocator alloc_others;
        // FIXME: free buffers after GPU works
        std::vector<BackendBuffer *> buffers;
        Backend * const backend;
    };

    class LayerAllocatorManager
    {
    public:
        enum MiscLayer
        {
            Prolog = -1,
            Epilog = -2,
        };

        void set_misc_layer_backend_mapping(int prolog, int epilog);

        void move_to_layer(int layer_id);

        int get_cur_layer(void) const;

        LayerBufAllocator *get_allocator(void);

        LayerBufAllocator *get_allocator(int layer_id);

    protected:
        int get_mapped_layer_id(int layer_id);
    public:
        std::vector<LayerBufAllocator> allocators;
    protected:
        int prolog_layer_backend_map_to_layer_id = -1;
        int epilog_layer_backend_map_to_layer_id = -1;
        int cur_layer = MiscLayer::Prolog;
    };

    class ComputeManager
    {
    public:
        static ggml_backend_allocator get_default_allocator_cpu(bool host_buffer, bool use_gpu);

        static ggml_backend_allocator get_default_allocator(ggml_backend_t backend);
        static int get_device_count(void);

        static ggml_backend_t init_backend_device(int index);

        static ggml_backend_allocator get_default_allocator_offload(int gpu);

        static size_t get_device_free_memory(int device, size_t *p_total = nullptr);

        static int get_max_devices(void);

        static bool is_gpu_offload_supported(void);
    };

    class Backend
    {
    public:
        Backend(ggml_backend_t backend, int n_layers, bool use_gpu);

        bool is_cpu(void) const;

        ggml_backend_allocator get_allocator(void);

        void write_tensor_data_async(ggml::tensor * tensor, const void * data, size_t offset, size_t size);

        static void write_tensor_data(ggml::tensor * tensor, const void * data, size_t offset, size_t size);

        static void write_tensor_data(ggml::tensor * tensor, const void * data);

        void read_tensor_data_async(ggml::tensor * tensor, void * data, size_t offset, size_t size);

        static void read_tensor_data(ggml::tensor * tensor, void * data, size_t offset, size_t size);

        static void read_tensor_data(ggml::tensor * tensor, void * data);

        void synchronize(void);

    public:
        ggml_backend_t backend;
        const int n_layers;
        const bool use_gpu;
    };

    class BackendContext
    {
    public:
        enum SplitMethod
        {
            None,
            Layer,
            Row,    // TODO: WIP. I can't figure out a _simple_ way for this yet.
        };

        struct gpu_cfg
        {
            int id;         // ignored for Metal
            int n_layers;
        };

        BackendContext();

        void init(const std::vector<gpu_cfg> &gpu_cfgs, const int n_layers, const size_t graph_max_nodes_num);

        ~BackendContext();

        bool reserve_memory(ggml_cgraph *gf);

        bool alloc_graph(ggml_cgraph *gf);

        void compute_graph(ggml_cgraph *gf, int n_threads);

        void reset();

        void set_abort_callback(struct llama_context *ctx, bool (*abort_callback)(void * data), void * abort_callback_data);

        void set_eval_observe_callback(ggml::need_observe_tensor_evaluation_callback need_observe_tensor_callback,
            ggml::observe_tensor_evaluation_callback observe_tensor_callback, void *user_data);

        void show_buffer_sizes(void);

    public:
        std::vector<Backend> backends;
    #ifdef GGML_USE_METAL
        ggml_backend_t backend_metal = nullptr;
    #endif
    #ifdef GGML_USE_BLAS
        ggml_backend_t backend_blas = nullptr;
    #endif
        ggml_backend_t backend_cpu = nullptr;

        // memory buffers used to evaluate the model
        std::vector<uint8_t> buf_compute_meta;
        ggml_backend_sched_t sched = nullptr;

        // host buffer for the model output (logits and embeddings)
        ggml_backend_buffer_t buf_output = nullptr;

        LayerAllocatorManager layer_allocators;
        LayerBufAllocator host_allocator;

    protected:
        ggml_abort_callback abort_callback      = nullptr;
        void *              abort_callback_data = nullptr;

    public:
        ggml::need_observe_tensor_evaluation_callback need_observe_tensor_callback = nullptr;
        ggml::observe_tensor_evaluation_callback      observe_tensor_callback = nullptr;
        void *                                        observe_tensor_callback_data = nullptr;
    };

    class ComputeContext
    {
    public:
        ComputeContext(BackendContext *backend_context);

        virtual struct ggml_context *get_ctx() = 0;
        virtual ggml_cgraph *get_cgraph(void);

        virtual void cb_new_tensor(ggml::tensor *tensor);
        virtual void cb_op_tensor(ggml::tensor *tensor);

        virtual void move_to_layer(int layer_id);

        BackendBufAllocator *get_allocator(void);

        virtual Backend *get_backend(void);

        virtual void compute(int n_threads);

        virtual bool allocate(void);

        virtual bool reserve_memory(void);

        virtual void reset(void);

        virtual size_t get_used_mem(void);
        virtual size_t get_mem_size(void);

        BackendContext *get_backend_context(void) { return backend_context; }

    protected:
        virtual ggml_backend_sched_t get_sched(void);

        BackendContext *backend_context;
    public:
        // obsoleted
        virtual void restart_scratch_alloc(void) {}
    private:
        void set_backend_context(BackendContext *backend_context);
    };
}