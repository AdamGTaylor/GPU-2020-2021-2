//i will use global atomics shown in atomics/atomics.cl
//after some reading, it turns out that local evaluations are cost effective and faster!
//oh, the histo_atomics has some issue to run it
//output: global memory
//input: the pic

//histo: local evaluation -> gets added to global in here
__kernel void gpu_histo_shared_atomics( __global unsigned int* output, __global int* input, int W )
{
    // histograms are in shared memory:
    __local unsigned int histo[256];

    // Number of threads in the block:
    int Nthreads = get_local_size(0) * get_local_size(1);
    // Linear thread idx:
    int LinID = get_local_id(0) + get_local_id(1) * get_local_size(0);

    // Zero histogram:
    for (int i = LinID; i < 256; i += Nthreads){ histo[i] = 0; }
    __syncthreads();

    // linear block index within 2D grid
    int B = get_group_id(0) + get_group_id(1) * get_num_groups(0);

    // process pixel blocks horizontally
    // updates the partial histogram in shared memory
    int y = get_group_id(1) * get_local_size(1) + get_local_id(1);
    //printf("%d : %d | %d : %d | W: %d\n", get_global_id(0), get_global_id(1),get_group_id(0),get_group_id(1), y * W + get_local_id(0));
    for (int x = get_local_id(0); x < W; x += get_local_size(0))
    {
        if(y < W){
            //printf("| %d | %d | %d |\n",y,y * W , y * W + x);
            int pixel = input[y * W + x];
            atomic_add(&histo[pixel], 1);
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    //Output index start for this block's histogram:
    int I = B*256;
    __global unsigned int* H = output + I;

    //Copy shared memory histograms to global memory:
    for (int i = LinID; i < 256; i += Nthreads)
    {
        H[i] = histo[i];
    }
}


//I still need you, as the blocks are holding my precious data in many histo ( or at least one)
__kernel void gpu_histo_accumulate(__global unsigned int* out, __global const unsigned int* in, int nBlocks)
{
    //Each thread sums one shade of the r, g, b histograms
    int i = get_global_id(0);
    if(i < 256)
    {
        unsigned int sum = 0;
        for(int j = 0; j < nBlocks; j++)
        {
            sum += in[i + 256 * j];
        }            
        out[i] = sum;
        //a strange debug tool, only works for low amount of data
        //printf("%d %u\n", i, out[i]);
    }
}

//cdf: sum[0,i]; 256 -> 256 (no partial sums -> global)
__kernel void gpu_global_cdf( __global unsigned int* output, __global const unsigned int* input)
{
    //Each thread will sum up till that poin
    int i = get_global_id(0);
    if(i < 256){
        unsigned int sum = 0;
        for(int j = 0; j <= i; ++j){
            //i only sum up nonzero elements -> an if here wouldn't be a problem
            sum += input[j];
        }
        output[i] = sum;
    }
}


//h_v
__kernel void gpu_global_h_v( __global int* output, __global int* input, int V_MIN , int size1, int size2)
{
    //creating h_v
    int i = get_global_id(0);
    if(i < 256){
        double num = 255*(input[i] - V_MIN)/(size1 * size2 - V_MIN);
        int n = (int)(num < 0 ? (num - 0.5) : (num + 0.5));
        //no need for atomic add
        output[i] = n;
    }
}

//eq_pic
__kernel void gpu_global_eq( __global unsigned int* output, __global unsigned int* in_pic, __global int* in_hv, int size1)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    output[col + row*size1] = in_hv[in_pic[col + row*size1]];
}