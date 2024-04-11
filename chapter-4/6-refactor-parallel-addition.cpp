#include <cassert>
#include <iostream>
#include <thread>
#include <random>
#include <limits>
#include <chrono>
#include <list>
#include <future>

/* Taken as a reference from the book for this exercise */
class vector 
{
  public:
    vector(int size) : my_size(size), data(new double[size]) {}

    vector() : my_size(0), data(0) {}

    ~vector() { delete[] data; }

    vector(const vector& that) 
      : my_size(that.my_size), data(new double[my_size])
    {
	for (int i=0; i<my_size; ++i) { data[i] = that.data[i]; }
    }

    vector& operator=(const vector& that) 
    {
	assert(that.my_size == my_size);
	for (int i=0; i<my_size; ++i) 
	    data[i] = that.data[i];
    return *this;
    }

    int size() const { return my_size; }
    int size() { return my_size; }

    double& operator[](int i) const {
	assert(i>=0 && i<my_size);
	return data[i];
    }

    double& operator[](int i) {
	assert(i>=0 && i<my_size);
	return data[i];

    }

    vector operator+(const vector& that) const {
	assert(that.my_size == my_size);
	vector sum(my_size);
	for (int i= 0; i < my_size; ++i)
	    sum[i] = (*this)[i] + that[i]; 
	return sum;
    }

  private:
    int     my_size;
    double* data;
};

std::ostream& operator<<(std::ostream& os, const vector& v)
{
  os << '[';
  for (int i= 0; i < v.size(); ++i) os << v[i] << ',';
  os << ']';
  return os;
}

double dot(const vector& v, const vector& w) 
{
    double s;
    for (int i= 0; i < v.size(); i++)
	s+= v[i] * w[i];
    return s;
}

/* simple randomization */
std::default_random_engine& global_urng() {
    static std::default_random_engine u{};
    return u;
}

void randomize() {
    static std::random_device rd{};
    global_urng().seed(rd());
}

int pick(int from, int thru) {
    static std::uniform_int_distribution<> d{};
    using parm_t = decltype(d)::param_type;
    return d(global_urng(), parm_t{from, thru});
}

vector vector_sum(const vector& lhs, const vector& rhs) {
    vector res(lhs.size());
    res = lhs + rhs;
    return res;
}

int main() {
    std::cout << std::thread::hardware_concurrency() << " threads available.\n";

    std::list<std::future<vector>> lf (std::thread::hardware_concurrency());

    vector sizes(4);
    sizes[0] = 10000; sizes[1] = 100000; sizes[2] = 1000000; sizes[3] = 10000000;
    
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

    for (int ati = 0; ati < std::thread::hardware_concurrency(); ati+=2) {
        for (int i = 0; i < sizes.size() - 1; i++) {
            vector v00(sizes[i]);
            vector v01(sizes[i]);
            vector res0(sizes[i]);
            vector v10(sizes[i+1]);
            vector v11(sizes[i+1]);
            vector res1(sizes[i+1]);

            for (int j = 0; j < sizes[i]; j++) {
                v00[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                v01[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                // v00[j] = pick(0, 10);
                // v01[j] = pick(0, 10);
            }
            for (int j = 0; j < sizes[i+1]; j++) {
                v10[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                v11[j] = pick(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                // v10[j] = pick(0, 10);
                // v11[j] = pick(0, 10);
            }

            lf.emplace_back(
                std::async(vector_sum, v00, v01)
            );
            lf.emplace_back(
                std::async(vector_sum, v10, v11)
            );
        }
    }

    auto end = std::chrono::steady_clock::now();
    
    const auto threadtime = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << "Whole calculation took " << threadtime << "us" << std::endl;

    for (auto it = lf.begin(); it != lf.end(); ++it) {
    // for (auto f : lf) {
        auto res = it->get();
        std::cout << "[" << res[0] << ", " << res[1] << ", ... , " << res[res.size()-2] << ", " << res[res.size()-1] << "]" << std::endl;
    }

    return 0;
}