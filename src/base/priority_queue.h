#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include<iostream>
#include<vector>

// max heap

// see <<Introduction to Algorithms>> 6.heapsort

template <typename T>
class PriorityQueue {
public:
    PriorityQueue(size_t maxSize) {
        heap.resize(maxSize);
        size = 0;
    }

    /*void Reserve(size_t size) {
        heap.resize(size);
    }*/

    void Clear(size_t size) {
        heap.resize(size);
        this->size = 0;
    }

    T Top() {
        return heap[0];
    }

    bool Empty() {
        return size <= 0;
    }

    size_t Size() {
        return size;
    }

    // ������heap�Ļ����ϰ�i����Ԫ�ص�������ȷ��λ��
    // �ҳ��������ģ��������»���
    void Heapify(size_t i) {
        size_t l = i * 2 + 1;
        size_t r = (i + 1) * 2;

        size_t largest = 0;

        if (l < size && heap[l] > heap[i]) // ��ߴ�
            largest = l;
        else
            largest = i;

        if (r < size && heap[r] > heap[largest]) // �ұߴ�
            largest = r;

        if (largest != i) { // ��ߴ���ұߴ�
            std::swap(heap[i], heap[largest]);
            Heapify(largest);
        }
    }

    // ����������鴴��max heap
    void BuildHeap() {
        for (size_t i = size / 2 - 1; i >= 0; --i) {
            Heapify(i);
        }
    }

    // ���һ���ŵ���һ��Heapify
    T Pop() {
        T max = heap[0];
        heap[0] = heap[size - 1];
        size--;
        Heapify(0);

        return max;
    }

    // �ŵ������������
    void Push(T t) {
        if (size >= heap.size()) {
            throw("PriorityQueue dead 1");
        }

        heap[size++] = t;

        size_t i = size - 1;

        while (i > 0 && heap[i] > heap[(i - 1) / 2]) { // û�����ұ�parent��
            std::swap(heap[i], heap[(i - 1) / 2]);
            i = (i - 1) / 2;
        }
    }
    

//private:
    std::vector<T> heap;

    size_t size;
};

#endif