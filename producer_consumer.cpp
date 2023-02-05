/**
 * @file producer_consumer.cpp
 * @author nanoshimarobot
 * @brief OperatingSystem midterm assignment
 * 
 */
#include <deque>
#include <string>
#include <random>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <iostream>

struct Data
{ // データ
    std::string creator_name_;
    int content;
};

class Buffer
{ // 共有資源
private:
    std::mutex mtx_;
    std::condition_variable condition_;
    std::deque<Data> resource_; // 実体
    size_t size_;

    std::unique_lock<std::mutex> get_lock() { return std::unique_lock<std::mutex>(mtx_); }

public:
    explicit Buffer(size_t size) : size_(size) { std::cout << "buffer created at size : " << size << std::endl; };

    void push_back(const Data &data)
    {
        std::unique_lock<std::mutex> lock = get_lock(); // lock
        condition_.wait(lock, [this]()
                        { return this->resource_.size() < this->size_; }); // キューが空くまで待機
        // if(resource_.size() >= size_) return;
        resource_.push_back(data);
        std::cout << "data " << data.content << " from " << data.creator_name_ << " was appended to buffer" << std::endl;
        condition_.notify_all();
    }

    Data pop_front(const std::string &thread_name)
    {
        std::unique_lock<std::mutex> lock = get_lock();
        condition_.wait(lock, [this]()
                        { return !this->resource_.empty(); }); // キューにデータが追加されるまで待機
        // while(resource_.empty());
        Data ret = resource_.front();
        resource_.pop_front();
        std::cout << "Front data " << ret.content << " created " << ret.creator_name_ << " was taken out by " << thread_name << std::endl;
        condition_.notify_all();
        return ret;
    }
};

class Producer
{
private:
    Buffer &buffer_;
    std::string name_;
    std::uniform_int_distribution<> rand_;
    std::random_device seed_gen_;
    std::mt19937 rand_engine_;
    static std::atomic<int> data;
    size_t task_amount_; // 生産回数

public:
    Producer(Buffer &buf, size_t task_amount, std::string name) : buffer_(buf), task_amount_(task_amount), name_(name), rand_{0, 1000}
    {
        rand_engine_.seed(seed_gen_());
    }

    void produce()
    {
        for (size_t i = 0; i < task_amount_; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{rand_(rand_engine_)});
            Data input;
            input.content = data++;
            input.creator_name_ = name_;
            buffer_.push_back(input);
        }
        std::cout << name_ << " task finished" << std::endl;
    }
};

std::atomic<int> Producer::data = 0;

class Consumer
{
private:
    Buffer &buffer_;
    std::string name_;
    std::uniform_int_distribution<> rand_;
    std::mt19937 rand_engine_;
    std::random_device seed_gen_;
    static std::atomic<int> data;
    size_t task_amount_; // 消費回数

public:
    Consumer(Buffer &buf, size_t task_amount, std::string name) : buffer_(buf), task_amount_(task_amount), name_(name), rand_{0, 1000}
    {
        rand_engine_.seed(seed_gen_());
    }

    void consume()
    {
        for (size_t i = 0; i < task_amount_; ++i)
        {
            const Data &data = buffer_.pop_front(name_);
            std::this_thread::sleep_for(std::chrono::milliseconds{rand_(rand_engine_)});
        }
        std::cout << name_ << " task finished" << std::endl;
    }
};

int main(void)
{
    std::cout << "start" << std::endl;
    Buffer buffer(5); // Bufferサイズを5

    Producer producer_1(buffer, 10, "producer_1");
    Producer producer_2(buffer, 10, "producer_2");
    Consumer consumer_1(buffer, 10, "consumer_1");
    Consumer consumer_2(buffer, 10, "consumer_2");

    std::thread producer_1_t([&]
                             { producer_1.produce(); });
    std::thread producer_2_t([&]
                             { producer_2.produce(); });
    std::thread consumer_1_t([&]
                             { consumer_1.consume(); });
    std::thread consumer_2_t([&]
                             { consumer_2.consume(); });

    producer_1_t.join();
    producer_2_t.join();
    consumer_1_t.join();
    consumer_2_t.join();

    std::cout << "finished" << std::endl;
}