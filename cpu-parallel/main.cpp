#include <iostream>
#include <cmath>
#include <fstream>

//for parallellization
#include <vector>
#include <random>
#include <numeric>
#include <future>

int size1 = 0;
int size2 = 0;
int count = 0;
int V_MIN = 0;

int low = 0;
int high = 255;

int it0 = 0;
int it1 = 0;

static const int histo_size = 256;

//this is a very good struct!
template <typename T>
struct atomic_wrapper
{
  std::atomic<T> data;

  atomic_wrapper():data(){}
  atomic_wrapper(std::atomic<T> const& a   ):data(a.load()){}
  atomic_wrapper(atomic_wrapper const& copy):data(copy.data.load()){}
  atomic_wrapper& operator=(atomic_wrapper const& copy){ data.store(copy.data.load()); return *this; }
};

template<typename T>
void save_pic(const std::vector<T> & veced_pic){
    int count = 0;
    std::ofstream my_file("../../../output/big_pic.txt");
    my_file << size1 << " " << size2 << "\n";
    for(int i=0; i!=size1; ++i){
        for(int j=0;j!=size2-1; ++j){
            my_file << veced_pic[i*size1 + j] << " ";
            count++;
        }
        my_file << veced_pic[i*size1 + size2 -1] << "\n";
        count++;
    }
}

template<typename T>
T summer(const std::vector<atomic_wrapper<T>> & veced_data,int i1,int i2){
    int sum = 0;
    for(int i=i1; i<=i2; ++i) sum+=veced_data[i].data.load();
    return sum;
}

//the main thing herec compared to basic is that i hae multiple threads
//this causes a bit of an issue as the first hist has to managed
//to not be accessed to write in it by multiple threads
// -> make a vector with atomic.
//no other element really requires as reading can be done by multiple threads
//no other element uses a higher element count object as its source of creation

// ------------------------------------------------------------------------
//i have to see how many threads i have -> it0 and it1 to these "bins"
//each thread always counts in this bin, never in other.

int main(int, char**) {
    //print program title
    std::cout << "Basic Histogramm Equalizer Started.\n";
    //init
    std::vector<int> pic;               //input pic
    std::vector<int> eq_pic;            //output pic

    std::vector<atomic_wrapper<int>> atomic_histogram(256); //his of pic
    std::vector<int> cdf(256,0);        //point mass function of pic
    std::vector<int> h_v(256,0);        //new values for eq_pic created from eq

    //i need dem threads!
    int max_num_of_threads = (int)std::thread::hardware_concurrency();
    std::vector<std::future<void>> atomic_futures(max_num_of_threads);
	std::cout << "Using " << max_num_of_threads << " threads.\n";


    //loading in texted picture
    std::ifstream myfile("../../../pics/big_pic.txt");
    if ( myfile.is_open() ){
        myfile >> size1;
        myfile >> size2;
        pic.resize(size1*size2);
        eq_pic.resize(size1*size2);
        for(int i=0; i < size1 * size2; ++i){
            myfile >> pic[i];
        }
        std::cout << "Succesful loading:" << size1 << "x" << size2 << std::endl;
    }

    //"lambdas"
    //histogramm from pic
    auto atomic_hist = [&](auto it0, auto it1){
        for(auto it=it0; it!=it1;++it){
            //as i take it from the pic, one cannot get values outside of [0,255]
            // no if...
            atomic_histogram[size_t(*it)].data += 1;
        }
    };
    //cdf maker
    auto gen_cdf = [&](auto it0, auto it1){
        for(auto it=it0; it!=it1;++it){
            //ISSUE: i hav to add +1 for some reason so i made a mistake somewhere
            //FOUND: instead of i!=i2 -> i=<i2, this way it steps on the [0,it]
            //       instead of [0,it) (becomes incluse)
            cdf[it] = summer(atomic_histogram,0,it+1);
        }
    };
    

    //h_b here
    auto h_v_maker = [&](auto it0, auto it1){
        for(auto it=it0; it!=it1;++it){
            int nominator = cdf[it]- V_MIN;
            int denominator = (size1 * size2 - V_MIN);
            h_v[it] = (int)round(255*nominator/denominator);
        }
    };

    auto equalizer = [&](auto it0, auto it1){
        for(auto it=it0; it!=it1;++it){
            eq_pic[it] = h_v[pic[it]];
        }
    };

    //calculation
    auto time0 = std::chrono::high_resolution_clock::now();

    //multithreaded
    //multithread: it from [it0,it1)
    //pmf
    for(int n=0; n<max_num_of_threads; ++n )
	{
		auto itv0 = pic.begin() + n * pic.size() / max_num_of_threads;
		auto itv1 = pic.begin() + (n + 1) * pic.size() / max_num_of_threads;
		atomic_futures[n] = std::async( std::launch::async, atomic_hist, itv0, itv1 );
	}
    //wait on futures and add results when it is possible
    std::for_each(atomic_futures.begin(), atomic_futures.end(), [](std::future<void>& f){ f.get(); } );
    

    //cdf
    for(int n=0; n<max_num_of_threads;++n)
    {
        //for some reason the atomic_wrapper vector is not liked by this.
        it0 = low + n * high / max_num_of_threads;
		it1 = low + (n + 1) * high / max_num_of_threads;
		atomic_futures[n] = std::async( std::launch::async, gen_cdf, it0, it1 );
    }
    std::for_each(atomic_futures.begin(), atomic_futures.end(), [](std::future<void>& f){ f.get(); } );
    
    //cdf -> amount of minimum value pixels
    bool found = true;
    for(int i=0; found && i!=cdf.size();++i){
        if(atomic_histogram[i].data!=0){
            V_MIN = cdf[i];
            found = false;
        }
    }
    
    //H_V making
    for(int n=0; n<max_num_of_threads;++n)
    {   
        //as it was pointed out, this is a bit problematic
        //int basicly just cuts off the "extra" present in double or float
        // (12.3 -> 12; 12.6 -> 12, maybe round?)
        it0 = low + n * high / max_num_of_threads;
		it1 = low + (n + 1) * high / max_num_of_threads;
		atomic_futures[n] = std::async( std::launch::async, h_v_maker, it0, it1 );
    }
    std::for_each(atomic_futures.begin(), atomic_futures.end(), [](std::future<void>& f){ f.get(); } );
    //equalizing
    for(int n=0; n<max_num_of_threads;++n)
    {
        it0 = low + n * size1*size2 / max_num_of_threads;
		it1 = low + (n + 1) * size1 * size2 / max_num_of_threads;
		atomic_futures[n] = std::async( std::launch::async, equalizer, it0, it1 );
    }
    std::for_each(atomic_futures.begin(), atomic_futures.end(), [](std::future<void>& f){ f.get(); } );
    

    auto time1 = std::chrono::high_resolution_clock::now();
    std::cout << "Histogram Equalized under " << std::chrono::duration_cast<std::chrono::nanoseconds>(time1-time0).count()/pow(10,6) << " msec!"  << std::endl;    save_pic(eq_pic);
    save_pic(eq_pic);

    return 0;
}
