#pragma once

#include <vector>
#include <mutex>

//UTILISE PAR LES SYSTEMES DE L'ECS
template <typename T>
class ArrayList {
public:
    std::vector<T> v{};
    /* ArrayList() : v(MEAN_ENTITIES) {
     }*/

     //avec un move ca irait un peu plus vite, j'ai pas testé avec  const& ???
    void insert(T val) {
        v.push_back(val);
    }
    void erase(T const& val) {
        for (unsigned int i = 0; i < v.size() - 1; i++) {
            if (v[i] == val) {
                v[i] = v[v.size() - 1];
                break;
            }
        }
        v.pop_back();
    }
};

template<typename T>
inline void removeFromVector(std::vector<T>& vec, int ind) {
    int lastIndex = vec.size() - 1;
    if (ind != lastIndex) {
        vec[ind] = std::move(vec[lastIndex]);
    }
    vec.pop_back();
}
template<typename T>
inline void removeFromVector(std::vector<T>* vec, int ind) {
    int lastIndex = vec->size() - 1;
    if (ind != lastIndex) {
        (*vec)[ind] = (*vec)[lastIndex];
    }
    vec->pop_back();
}

//UNUSED
template<typename T>
class MonoConsummerAndProducerConcurrentList {
	struct Node {
		T* value;
		Node* next;
	};
	Node* head{ nullptr };
	Node* tail{ nullptr };
	std::unique_lock<std::mutex> lock{};
	T* deq() {
		if (head == nullptr)
			return nullptr;

		if (head == tail || head->next == tail) {
			lock.lock();
			T* valToReturn = head->value;
			Node* next = head->next;
			delete head;
			head = next;
			lock.unlock();
			return valToReturn;
		}
		else {
			T* valToReturn = head->value;
			Node* next = head->next;
			delete head;
			head = next;
			return valToReturn;
		}
	}
	void enq(T* val) {
		lock.lock();
		if (tail == nullptr) {

		}
		tail->next = new Node{ val, nullptr };
		tail = tail->next;
		lock.unlock();
	}
};

//WIP
//2 solutions to compare:
//1: completely lock the queue when read/write
//2: use atomic indices for the reader and the writer => nah dont work :(

//This is a circular queue used for the TCP chat message, using a array with contiguous memory storage
//1 network thread will only enqueue messages while 1 main thread will read messages at any indices
//If the max size is reached the oldest message is removed
//Concurrency: only 1 writer thread and 1 reader thread !
template<typename T>
class ConcurrentCircularQueue {
private:
	std::mutex queueMutex;
	std::unique_lock<std::mutex> lock;

public:
	std::vector<T> v;
	//queue empty <=> head = tail
	//queue full <=> head = tail + 1 (mod n)
	int head{ 0 };
	int tail{ 0 };
	ConcurrentCircularQueue(int maxElements) : v(maxElements + 1), queueMutex{}, lock(queueMutex, std::defer_lock) {
	}
	ConcurrentCircularQueue() = delete;
	
	void lockQueue() {
		lock.lock();
	}
	void length() const {
		int len = (tail - head + 1) % v.size();
		if (len < 0)
			len += v.size();
		return len;
	}
	bool empty() const {
		return head == tail;
	}

	void unlockQueue() {
		lock.unlock();
	}

	//index should be between 0 and maxElements-1
	//MUST not be called before startReading (in the same thread)
	T const& read(int relativeIndexToRead) {
		//readerIndex.store((relativeIndexToRead + head) % v.size());
		//block while another thread is writing at the same index
		//while (writerIndex.load() == readerIndex.load()) {};
		return v[(relativeIndexToRead + head) % v.size()];
	}
	//will delete head element if max size is reached
	void enqueue(T&& elementToEnq) {
		//writerIndex.store((tail + 1) % v.size());
		//block while another thread is reading at the same index
		//while (writerIndex.load() == readerIndex.load()) {};

		//writerIndex.store(-1);
		v[tail] = std::forward<T>(elementToEnq);

		tail = (tail + 1) % v.size();
		//if queue is full dequeue head
		if (tail == head) {
			head = (head + 1) % v.size();
		}
	}
	void dequeue() {
		//if queue is not empty
		if (tail != head) {
			head = (head + 1) % v.size();
		}
	}
};


class UniqueByteArray {
private:
	char* data;//could be std::byte* ?
public:
	UniqueByteArray(size_t initialSize) : data{nullptr} {
		data = reinterpret_cast<char*> (std::malloc(sizeof(char)*initialSize));
		if (data == nullptr)
			throw std::bad_alloc();
	}
	void realloc(size_t newSize) {
		char* newPtr{ static_cast<char*>(std::realloc(data, sizeof(char)*newSize)) };
		if (newPtr == nullptr) {
			std::free(data);
			throw std::bad_alloc();
		}
		data = newPtr;
	}
	char* get() const noexcept {
		return data;
	}

	~UniqueByteArray() noexcept {
		if (data != nullptr)//data can be nulltr if the object data has been moved
			std::free(data);
	}
	UniqueByteArray(UniqueByteArray const& other) = delete;//remove cpy constructor
	UniqueByteArray& operator=(UniqueByteArray const& other) = delete;//remove cpy assignment
	UniqueByteArray(UniqueByteArray&& other) noexcept :
		data{ std::move(other.data) }
	{
		other.data = nullptr;
	}
	UniqueByteArray& operator=(UniqueByteArray&& other) noexcept {
		data = std::move(other.data);
		other.data = nullptr;
		return *this;
	}

};