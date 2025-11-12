#include <gtest/gtest.h>

#include "mem_res.hpp"     
#include "queue.hpp"  

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <type_traits>

TEST(PmrQueueBasicInt, PushPopAndOrder) {
    StaticVectorBlocks pool(64 * 1024);
    PmrQueue<int> q(4, &pool);

    EXPECT_TRUE(q.empty());
    q.push(10);
    q.push(20);
    q.emplace(30);
    EXPECT_EQ(q.size(), 3u);

    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 30);

    q.pop();
    EXPECT_EQ(q.front(), 20);
    q.pop();
    EXPECT_EQ(q.front(), 30);
    q.pop();
    EXPECT_TRUE(q.empty());
}

TEST(PmrQueueIterator, IterateOverElements) {
    StaticVectorBlocks pool(64 * 1024);
    PmrQueue<int> q(4, &pool);
    q.push(1);
    q.push(2);
    q.push(3);

    std::vector<int> out;
    for (auto it = q.begin(); it != q.end(); ++it) {
        out.push_back(*it);
    }
    EXPECT_EQ(out, std::vector<int>({1,2,3}));

    const PmrQueue<int>& cq = q;
    std::vector<int> out2;
    for (auto it = cq.begin(); it != cq.end(); ++it) out2.push_back(*it);
    EXPECT_EQ(out2, out);
}

TEST(PmrQueueComplexType, WorksWithPmrString) {
    StaticVectorBlocks pool(64 * 1024);

    struct Complex {
        int id;
        double v;
        std::pmr::string name;
        Complex(int i, double vv, std::pmr::string n) : id(i), v(vv), name(std::move(n)) {}
    };

    PmrQueue<Complex> q(2, &pool);
    std::pmr::string s1("alpha", &pool);
    std::pmr::string s2("beta", &pool);

    q.emplace(1, 3.14, s1);
    q.emplace(2, 2.71, s2);

    ASSERT_EQ(q.size(), 2u);
    EXPECT_EQ(q.front().id, 1);
    EXPECT_DOUBLE_EQ(q.front().v, 3.14);
    EXPECT_EQ(q.front().name, "alpha");

    EXPECT_EQ(q.back().id, 2);
    EXPECT_EQ(q.back().name, "beta");

    q.pop();
    EXPECT_EQ(q.front().id, 2);
    EXPECT_EQ(q.front().name, "beta");
    q.pop();
    EXPECT_TRUE(q.empty());
}

TEST(PmrQueueGrowth, ReallocateAndPreserveOrder) {
    StaticVectorBlocks pool(128 * 1024);
    PmrQueue<int> q(2, &pool);

    for (int i = 0; i < 20; ++i) q.push(i);

    EXPECT_EQ(q.size(), 20u);

    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(q.front(), i);
        q.pop();
    }
    EXPECT_TRUE(q.empty());
}

TEST(PmrQueueMemoryReuse, PushPopPush_NoBadAlloc) {
    StaticVectorBlocks pool(8 * 1024);

    PmrQueue<int> q(4, &pool);

    for (int i = 0; i < 8; ++i) q.push(i);
    EXPECT_EQ(q.size(), 8u);

    for (int i = 0; i < 8; ++i) q.pop();
    EXPECT_TRUE(q.empty());

    for (int i = 0; i < 8; ++i) {
        EXPECT_NO_THROW(q.push(i + 100));
    }

    for (int i = 0; i < 8; ++i) {
        EXPECT_EQ(q.front(), i + 100);
        q.pop();
    }
}

TEST(PmrQueueCopying, DeepCopyWorks) {
    StaticVectorBlocks pool(64 * 1024);
    PmrQueue<int> a(4, &pool);
    a.push(1); a.push(2); a.push(3);

    PmrQueue<int> b = a;
    EXPECT_EQ(a.size(), b.size());

    a.pop();
    a.push(99);

    std::vector<int> va, vb;
    for (auto it = a.begin(); it != a.end(); ++it) va.push_back(*it);
    for (auto it = b.begin(); it != b.end(); ++it) vb.push_back(*it);

    EXPECT_NE(va, vb);
    EXPECT_EQ(vb, std::vector<int>({1,2,3}));
}

TEST(PmrQueueExhaustion, SmallPoolThrowsBadAlloc) {
    StaticVectorBlocks tiny(16); 

    EXPECT_THROW({
        PmrQueue<int> q(8, &tiny); 
    }, std::bad_alloc);
}

static_assert(std::is_same_v<typename PmrQueue<int>::iterator::iterator_category, std::forward_iterator_tag>,
              "iterator must be forward_iterator_tag");

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
