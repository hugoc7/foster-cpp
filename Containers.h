#pragma once

#include <vector>
#include <mutex>
#include <cstddef>
#include "SDL_Net.h"
#include "struct/struct.h"
#include <cassert>


//UTILISE PAR LES SYSTEMES DE L'ECS
template <typename T>
class ArrayList {
public:
    std::vector<T> v{};
    /* ArrayList() : v(MEAN_ENTITIES) {
     }*/

     //avec un move ca irait un peu plus vite, j'ai pas test� avec  const& ???
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
	int length() const {
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
	T const& read(int relativeIndexToRead) const {
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


template<typename T>
class CircularQueue {
private:
	std::vector<T> v;
	//queue empty <=> head = tail
	//queue full <=> head = tail + 1 (mod n)
	int head{ 0 };
	int tail{ 0 };
public:


	CircularQueue(size_t maxElements) : v(maxElements + 1u) {
	}
	CircularQueue() = delete;
	int length() const {
		if (head <= tail) {
			return tail - head;
		}
		else {
			return v.size() - head + tail;
		}
	}
	inline bool empty() const noexcept {
		return head == tail;
	}
	inline bool full() const {
		return (head == (tail + 1) % v.size());
	}

	T const& read(int relativeIndexToRead) const {
		int realIndex = (relativeIndexToRead + head) % v.size();
		assert(realIndex >= 0 && realIndex < v.size());
		return v[realIndex];
	}
	//will delete head element if max size is reached
	void enqueue(T&& elementToEnq) {
		if (full()) {
			dequeue();
		}
		assert(tail >= 0 && tail < v.size());
		v[tail] = std::forward<T>(elementToEnq);
		tail = (tail + 1) % v.size();
		assert(tail >= 0 && tail < v.size());

	}
	void dequeue() {
		//if queue is not empty
		if (!empty()) {
			head = (head + 1) % v.size();
		}
	}
};


class UniqueByteBuffer {
private:
	Uint8* data{NULL};//could be std::byte* ?
	unsigned int size{0};
public:
	UniqueByteBuffer(unsigned int initialSize) :
		data{ reinterpret_cast<Uint8*> (std::malloc(initialSize)) },
		size{ initialSize }
	{
		assert(initialSize >= 1);
		if (data == nullptr)
			throw std::bad_alloc();
	}
	unsigned int Size() const noexcept {
		return size;
	}/*
	void Write(Uint16 n, int pos) {
		assert(pos >= 0 && pos < size&& data != nullptr);
		SDLNet_Write16(n, &data[pos]);
	}
	void Write(Uint32 n, int pos) {
		assert(pos >= 0 && pos < size&& data != nullptr);
		SDLNet_Write32(n, &data[pos]);
	}
	void Write(float f, int pos) {
		assert(pos >= 0 && pos + 3 < size && data != nullptr);
		struct_pack(&data[pos], "f", f);
	}
	float ReadFloat(int pos) {
		assert(pos >= 0 && pos + 3 < size && data != nullptr);
		float res;
		struct_unpack(&data[pos], "f", &res);
		return res;
	}
	Uint32 ReadUint32(int pos) {
		assert(pos >= 0 && pos < size&& data != nullptr);
		return SDLNet_Read32(&data[pos]);
	}
	Uint16 ReadUint16(int pos) {
		assert(pos >= 0 && pos < size&& data != nullptr);
		return SDLNet_Read16(&data[pos]);
	}*/
	void realloc(unsigned int newSize) {
		assert(newSize >= 1);
		Uint8* newPtr{ static_cast<Uint8*>(std::realloc(data, newSize)) };
		if (newPtr == nullptr) {
			std::free(data);
			throw std::bad_alloc();
		}
		data = newPtr;
		size = newSize;
	}
	void lossfulRealloc(unsigned int newSize) {
		assert(newSize >= 1);
		std::free(data);
		data = reinterpret_cast<Uint8*> (std::malloc(newSize));
		if (data == nullptr)
			throw std::bad_alloc();
		size = newSize;
	}
	Uint8* get() const noexcept {
		assert(data != NULL);
		return data;
	}

	~UniqueByteBuffer() noexcept {
		if (data != nullptr)//data can be nulltr if the object data has been moved
			std::free(data);
	}
	UniqueByteBuffer(UniqueByteBuffer const& other) = delete;//remove cpy constructor
	UniqueByteBuffer& operator=(UniqueByteBuffer const& other) = delete;//remove cpy assignment
	UniqueByteBuffer(UniqueByteBuffer&& other) noexcept :
		data{ std::move(other.data) },
		size{ other.size }
	{
		other.data = nullptr;
	}
	UniqueByteBuffer& operator=(UniqueByteBuffer&& other) noexcept {
		data = std::move(other.data);
		other.data = nullptr;
		return *this;
	}

};

//A sparsed vector = un vecteur � trous
//template<typename T>
class SparsedIndicesVector {
private:
	std::vector<int> vec;
	int size{0};
public:
	SparsedIndicesVector(int capacity) {
		vec.reserve(capacity);
		for (int i = 0; i < vec.size(); i++) {
			vec.push_back(i);
		}
	}
	//no modification possible
	int operator[](int pos) const {
		return vec[pos];
	}
	int Size() const {
		return size;
	}
	int Add() {
		if (size < vec.size()) {
			return vec[size++];
		}
		else {
			assert(size == vec.size());
			vec.push_back(size);
			return size++;
		}
	}
	void Remove(int index) {
		assert(index < size && !vec.empty());
		int realIndex = 0;
		while (vec[realIndex] != index) {
			realIndex++;
			assert(realIndex < size);
		}
		if (realIndex != size - 1) {
			vec[realIndex] = vec[size - 1];
			vec[size - 1] = index;
		}
		size--;
	}
};

//K = key type, should be a number
//V = value type, can be anything
//description: a map (dictionnary) implemented with an array. The keys are positive integers between 0 and a maximum value. (they are indices of a vector)
template <typename V, typename K = Uint16> 
class ArrayMap {
protected:
	std::vector<int> index;
	std::vector<V> values;
public:
	static const int EMPTY = -1;
	static const int MAX_KEYS = 1000;

	inline bool IsValidKey(K key) const noexcept {
		return 0 <= key && key <= MAX_KEYS;
	}

	template<typename ... Args>
	//key must be valid and should not already exist in the map
	void Add(K key, Args&& ... args) {
		assert(0 <= key && key <= MAX_KEYS);//invalid key
		values.emplace_back(std::forward<Args>(args)...);
		if (index.size() <= key)
			index.resize(key+1, ArrayMap::EMPTY);

		index[key] = values.size() - 1;
	}
	//key must be valid
	template<typename ... Args>
	void AddOrReplace(K key, Args&& ... args) {
		assert(0 <= key && key <= MAX_KEYS);//invalid key
		if (key < index.size() && index[key] >= 0 && index[key] < values.size()) {
			//replace
			V newValue(std::forward<Args>(args)...);
			values[index[key]] = std::move(newValue);
			return;
		}

		//add
		values.emplace_back(std::forward<Args>(args)...);
		if (index.size() <= key)
			index.resize(key + 1, ArrayMap::EMPTY);

		index[key] = values.size() - 1;
	}
	inline size_t Size() const {
		return values.size();
	}
	V const& operator[](int index) const {
		return values[index];
	}
	//returns false if the key is invalid or not contained in the map
	bool Contains(K key) const noexcept {
		return key >= 0 && key < index.size() && index[key] >= 0 && index[key] < values.size();
		//note that index[key] should ALWAYS be equal to EMPTY (-1) if the key dont exist 
	}
	//WARNING: if key is invalid or dont exist => undefined behaviour
	void Remove(K key) {
		assert(0 <= key && key < index.size());
		removeFromVector(values, index[key]);
		index[key] = ArrayMap::EMPTY;
	}
	//WARNING: if key is invalid or dont exist => undefined behaviour
	V& Get(K key) {
		assert(0 <= key && key < index.size());
		return values[index[key]];
	}
	void Clear() {
		values.clear();
		index.clear();
	}
};