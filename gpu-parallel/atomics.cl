//i will use global atomics shown in atomics/atomics.cl
//after some reading, it turns out that local evaluations are cost effective and faster!
__kernel void gpu_histo_shared_atomics( __global unsigned int* output, __global int* input, int W )
{
    //histograms are in shared memory:
    __local unsigned int histo[256];

    //Number of threads in the block:
    int Nthreads = get_local_size(0) * get_local_size(1);
    //Linear thread idx:
    int LinID = get_local_id(0) + get_local_id(1) * get_local_size(0);
    //zero histogram:
    for (int i = LinID; i < 256; i += Nthreads){ histo[i] = 0; }
    __syncthreads();

    // linear block index within 2D grid
    int B = get_group_id(0) + get_group_id(1) * get_num_groups(0);

    // process pixel blocks horizontally
    // updates the partial histogram in shared memory
    int y = get_group_id(1) * get_local_size(1) + get_local_id(1);
    for (int x = get_local_id(0); x < W; x += get_local_size(0))
    {
        int pixel = input[y * W + x];
        atomic_add(&histo[pixel], 1);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    //Output index start for this block's histogram:
    int I = B*256;
    __global unsigned int* H = output + I;

    //Copy shared memory histograms to globl memory:
    for (int i = LinID; i < 256; i += Nthreads)
    {
        H[i] = histo[i];
    }
}

__kernel void gpu_histo_accumulate(__global unsigned int* out, __global const unsigned int* in, int nBlocks)
{
    //each thread sums one shade of the r, g, b histograms
    int i = get_global_id(0);
    if(i < 256)
    {
        unsigned int sum = 0;
        for(int j = 0; j < nBlocks; j++)
        {
            sum += in[i + 256 * j];
        }            
        out[i] = sum;
    }
}
