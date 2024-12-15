#ifndef HPX_WRAPPER_HPP
#define HPX_WRAPPER_HPP

#include <hpx/hpx.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

/**
 * @brief Sorts the given array of integers in ascending order.
 *
 * @param src Pointer to the input array of int32_t elements.
 * @param size Number of elements in the input array.
 * @return A future that, when ready, returns a shared pointer to a sorted vector<int32_t>.
 *         The returned vector is a copy of the input data, sorted in ascending order.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_sort(const int32_t* src, size_t size);

/**
 * @brief Counts the number of occurrences of a given value in the array.
 *
 * @param src Pointer to the input array of int32_t elements.
 * @param size Number of elements in the input array.
 * @param value The int32_t value to count.
 * @return A future that, when ready, returns the count of how many times 'value' appears.
 */
hpx::future<int64_t> hpx_count(const int32_t* src, size_t size, int32_t value);

/**
 * @brief Creates a copy of the given array.
 *
 * @param src Pointer to the input array of int32_t elements.
 * @param size Number of elements in the input array.
 * @return A future that, when ready, returns a shared pointer to a copy of the input vector.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy(const int32_t* src, size_t size);

/**
 * @brief Checks if the array 'src' ends with the sequence 'suffix'.
 *
 * @param src Pointer to the main array.
 * @param src_size Number of elements in the main array.
 * @param suffix Pointer to the suffix array.
 * @param suffix_size Number of elements in the suffix array.
 * @return A future that, when ready, returns true if 'src' ends with 'suffix', false otherwise.
 */
hpx::future<bool> hpx_ends_with(const int32_t* src, size_t src_size, const int32_t* suffix, size_t suffix_size);

/**
 * @brief Checks if two arrays are equal in size and element-wise comparison.
 *
 * @param arr1 Pointer to the first array.
 * @param size1 Number of elements in the first array.
 * @param arr2 Pointer to the second array.
 * @param size2 Number of elements in the second array.
 * @return A future that, when ready, returns true if both arrays are equal, false otherwise.
 */
hpx::future<bool> hpx_equal(const int32_t* arr1, size_t size1, const int32_t* arr2, size_t size2);

/**
 * @brief Finds the first occurrence of a given value in the array.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param value The int32_t value to find.
 * @return A future that, when ready, returns the index of the first occurrence of 'value', 
 *         or -1 if not found.
 */
hpx::future<int64_t> hpx_find(const int32_t* src, size_t size, int32_t value);

/**
 * @brief Merges two sorted arrays into a single sorted array.
 *
 * Both input arrays must be sorted in ascending order.
 *
 * @param src1 Pointer to the first sorted array.
 * @param size1 Number of elements in the first array.
 * @param src2 Pointer to the second sorted array.
 * @param size2 Number of elements in the second array.
 * @return A future that, when ready, returns a shared pointer to a merged, sorted vector<int32_t>.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_merge(const int32_t* src1, size_t size1, const int32_t* src2, size_t size2);

/**
 * @brief Partially sorts the array so that elements before 'middle' are in ascending order.
 *
 * This arranges the smallest 'middle' elements in ascending order at the start of the array,
 * while the remaining elements' order is unspecified beyond that.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param middle The position marking how many elements should be sorted from the start.
 * @return A future that, when ready, returns a shared pointer to a vector<int32_t>
 *         with the first 'middle' elements sorted.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_partial_sort(const int32_t* src, size_t size, size_t middle);

/**
 * @brief Copies the first 'count' elements of the input array into a new vector.
 *
 * @param src Pointer to the input array.
 * @param count The number of elements to copy.
 * @return A future that, when ready, returns a shared pointer to a vector<int32_t> with 'count' elements copied.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy_n(const int32_t* src, size_t count);

/**
 * @brief Creates a vector of given size, filling all elements with a specified value.
 *
 * @param value The int32_t value to fill.
 * @param size The number of elements in the resulting vector.
 * @return A future that, when ready, returns a shared pointer to a vector<int32_t> 
 *         filled entirely with 'value'.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_fill(int32_t value, size_t size);

/**
 * @brief Counts how many elements satisfy a given predicate function.
 *
 * The predicate is a C++ function<bool(int32_t)> that tests each element.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param pred A callable that takes an int32_t and returns true/false.
 * @return A future that, when ready, returns the count of elements for which 'pred' returns true.
 */
hpx::future<int64_t> hpx_count_if(const int32_t* src, size_t size, std::function<bool(int32_t)> pred);

/**
 * @brief Copies all elements that satisfy a given predicate into a new vector.
 *
 * The predicate is a C++ function<bool(int32_t)>, and only elements for which this 
 * returns true will appear in the resulting vector.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param pred A callable that takes an int32_t and returns true/false.
 * @return A future that, when ready, returns a shared pointer to a vector<int32_t>
 *         containing only the elements that satisfy 'pred'.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_copy_if(const int32_t* src, size_t size, std::function<bool(int32_t)> pred);

/**
 * @brief Sorts the array according to a custom comparator function.
 *
 * The comparator is a function<bool(int32_t,int32_t)> that defines ordering between elements.
 * The array is sorted based on this comparator.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param comp A callable that takes two int32_t elements and returns true if the first is "less" than the second.
 * @return A future that, when ready, returns a shared pointer to a sorted vector<int32_t>, 
 *         sorted according to 'comp'.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_sort_comp(const int32_t* src, size_t size, std::function<bool(int32_t,int32_t)> comp);

/**
 * @brief Partially sorts the array using a custom comparator, ensuring the first 'middle' elements
 *        are in ascending order according to 'comp'.
 *
 * Similar to hpx_partial_sort, but uses a user-defined comparator.
 *
 * @param src Pointer to the input array.
 * @param size Number of elements in the input array.
 * @param middle The number of smallest elements to sort to the front of the array.
 * @param comp A comparator function<bool(int32_t,int32_t)>.
 * @return A future that, when ready, returns a shared pointer to a vector<int32_t>
 *         with the first 'middle' elements sorted according to 'comp'.
 */
hpx::future<std::shared_ptr<std::vector<int32_t>>> hpx_partial_sort_comp(const int32_t* src, size_t size, size_t middle, std::function<bool(int32_t,int32_t)> comp);

#endif // HPX_WRAPPER_HPP
