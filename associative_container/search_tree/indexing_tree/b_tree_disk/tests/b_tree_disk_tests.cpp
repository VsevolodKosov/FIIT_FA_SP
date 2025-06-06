#include "gtest/gtest.h"
#include "b_tree_disk.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

template<typename tkey, typename tvalue>
bool compare_results(
        const std::vector<std::pair<tkey, tvalue> > &expected,
        const std::vector<std::pair<tkey, tvalue> > &actual) {
    if (expected.size() != actual.size()) return false;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i].first != actual[i].first || expected[i].second != actual[i].second) {
            return false;
        }
    }
    return true;
}

class BTreeDiskTest : public ::testing::Test {
protected:
    std::string test_file;

    void SetUp() override {
        // test_file должен быть установлен в каждом тесте до вызова SetUp
        // fs::remove(test_file + ".tree");
        // fs::remove(test_file + ".data");
    }

    void TearDown() override {
        // fs::remove(test_file + ".tree");
        // fs::remove(test_file + ".data");
    }
};

TEST_F(BTreeDiskTest, InsertAndFind) {
    test_file = "test_tree_insert";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    B_tree_disk<SerializableInt, SerializableString> tree(test_file);

    EXPECT_TRUE(tree.insert({SerializableInt{1}, SerializableString{"a"}}));
    EXPECT_TRUE(tree.insert({SerializableInt{2}, SerializableString{"b"}}));

    auto val = tree.at(SerializableInt{1});
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(val.value().get(), "a");
}

TEST_F(BTreeDiskTest, RangeSearch) {
    test_file = "test_tree_range";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    B_tree_disk<SerializableInt, SerializableString> tree(test_file);

    tree.insert({SerializableInt{1}, SerializableString{"a"}});
    tree.insert({SerializableInt{2}, SerializableString{"b"}});
    tree.insert({SerializableInt{3}, SerializableString{"c"}});
    tree.insert({SerializableInt{4}, SerializableString{"d"}});

    auto [begin, end] = tree.find_range(SerializableInt{2}, SerializableInt{4});
    std::vector<std::pair<SerializableInt, SerializableString> > result;
    while (begin != end) {
        result.push_back(*begin);
        ++begin;
    }

    std::vector<std::pair<SerializableInt, SerializableString> > expected = {
            {SerializableInt{2}, SerializableString{"b"}},
            {SerializableInt{3}, SerializableString{"c"}},
            {SerializableInt{4}, SerializableString{"d"}}
    };

    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(result[i].first.data, expected[i].first.data);
        EXPECT_EQ(result[i].second.data, expected[i].second.data);
    }
}

TEST_F(BTreeDiskTest, Persistence) {
    test_file = "test_tree_persistence";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    {
        B_tree_disk<SerializableInt, SerializableString> tree(test_file);
        tree.insert({SerializableInt{1}, SerializableString{"a"}});
        tree.insert({SerializableInt{2}, SerializableString{"b"}});
    }

    B_tree_disk<SerializableInt, SerializableString> tree1(test_file);

    EXPECT_TRUE(tree1.at(SerializableInt{1}).has_value());
    EXPECT_TRUE(tree1.at(SerializableInt{2}).has_value());
}

TEST_F(BTreeDiskTest, ComplexStructure) {
    test_file = "test_tree_complex";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    B_tree_disk<SerializableString, SerializableString, std::less<SerializableString>, 3> tree(test_file);

    for (int i = 0; i < 100; ++i) {
        tree.insert({SerializableString{std::to_string(i)}, SerializableString{"value_" + std::to_string(i)}});
    }

    for (int i = 0; i < 100; ++i) {
        auto val = tree.at(SerializableString{std::to_string(i)});
        EXPECT_TRUE(val.has_value());
        EXPECT_EQ(val.value().get(), "value_" + std::to_string(i));
    }
}

TEST_F(BTreeDiskTest, EraseTest) {
    test_file = "test_tree_erase";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    B_tree_disk<SerializableInt, SerializableString, std::less<SerializableInt>, 3> tree(test_file);

    // 1. Insert test data
    for (int i = 0; i < 50; i++) {
        tree.insert({SerializableInt{i}, SerializableString{"value_" + std::to_string(i)}});
    }
    std::cout << "\n================= До удалений" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    // 2. Test basic deletion from a leaf
    EXPECT_TRUE(tree.erase(SerializableInt{20}));
    EXPECT_FALSE(tree.at(SerializableInt{20}).has_value());
    std::cout << "\n================= Попытался удалить 20" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    // 3. Test deleting non-existent key - should return false
    EXPECT_FALSE(tree.erase(SerializableInt{100}));
    std::cout << "\n================= Попытался удалить 100" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    // 4. Test deleting a key that causes rebalancing
    // First verify the key exists
    EXPECT_TRUE(tree.at(SerializableInt{5}).has_value());
    EXPECT_TRUE(tree.erase(SerializableInt{5}));
    EXPECT_FALSE(tree.at(SerializableInt{5}).has_value());
    std::cout << "\n================= Попытался удалить 5" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    // 5. Delete multiple keys to force tree restructuring
    for (int i = 10; i < 15; i++) {
        EXPECT_TRUE(tree.erase(SerializableInt{i}));
        EXPECT_FALSE(tree.at(SerializableInt{i}).has_value());
    }
    std::cout << "\n================= Попытался удалить [10-15)" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    std::cout << "\n=================" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    // 6. Verify remaining keys still work
    EXPECT_TRUE(tree.at(SerializableInt{}).has_value());
    EXPECT_EQ(tree.at(SerializableInt{}).value().data, "value_0");

    // 7. Delete the root key (if your tree structure allows identifying it)
    int root_key = 15; // This assumes 15 is a key in the root node based on the query
    EXPECT_TRUE(tree.erase(SerializableInt{root_key}));
    std::cout << "\n================= Удалил 15" << std::endl;
    tree.print();
    std::cout << "=================\n" << std::endl;
    EXPECT_FALSE(tree.at(SerializableInt{root_key}).has_value());
}

TEST_F(BTreeDiskTest, NegativeTest) {
    test_file = "test_tree_negative";
    fs::remove(test_file + ".tree");
    fs::remove(test_file + ".data");
    B_tree_disk<SerializableInt, SerializableString> tree(test_file);

    EXPECT_FALSE(tree.erase(SerializableInt{42}));
    EXPECT_FALSE(tree.at(SerializableInt{42}).has_value());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
