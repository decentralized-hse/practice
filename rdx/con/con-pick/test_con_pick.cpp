#include <gtest/gtest.h>
#include "con_pick.h"

// Тестируем функцию calculateDifficulty, которая считает нули в начале строки
TEST(CalculateDifficultyTest, ZeroCount) {
    EXPECT_EQ(calculateDifficulty("0000"), 4); // Строка из 4 нулей
    EXPECT_EQ(calculateDifficulty("abc"), 0); // Без нулей
    EXPECT_EQ(calculateDifficulty("000abc"), 3); // 3 нуля в начале
    EXPECT_EQ(calculateDifficulty(""), 0); // Пустая строка
}

TEST(CalculateDifficultyTest, LongZeroString) {
    std::string longZeros(1000, '0'); // Строка из 1000 нулей
    EXPECT_EQ(calculateDifficulty(longZeros), 1000); // Проверяем, что все нули посчитались
}

// Тестируем выбор лучшей цепочки в функции getBestChain
TEST(GetBestChainTest, SelectsBestChain) {
    std::vector<Block> blocks = {
        {"1", "", 3},
        {"2", "1", 2},
        {"3", "2", 1},
        {"4", "", 5}, // Блок без связей, отдельная цепочка
    };

    Block bestBlock = getBestChain(blocks, false);
    EXPECT_EQ(bestBlock.hash, "3"); // Блок "3" должен быть частью самой сложной цепочки (3+2+1=6).

    bestBlock = getBestChain(blocks, true); // Проверяем вариант с "вредоносной" логикой
    EXPECT_EQ(bestBlock.hash, "1");
}

TEST(GetBestChainTest, ComplexChains) {
    std::vector<Block> blocks = {
        {"1", "", 3},
        {"2", "1", 2},
        {"3", "2", 2},
        {"4", "3", 2},
        {"5", "", 5},  // Новый старт для другой цепочки
        {"6", "5", 1}
    };

    Block bestBlock = getBestChain(blocks, false);
    EXPECT_EQ(bestBlock.hash, "4"); // Цепочка 1-2-3-4 должна быть самой сложной (3+2+2+2=9).

    bestBlock = getBestChain(blocks, true);
    EXPECT_EQ(bestBlock.hash, "1"); // В "вредоносном" режиме цепочка 1 самая простая
}

// Пустой ввод для getBestChain
TEST(GetBestChainTest, EmptyData) {
    std::vector<Block> emptyBlocks;
    Block bestBlock = getBestChain(emptyBlocks, false);
    EXPECT_EQ(bestBlock.hash, ""); // Без блоков результат будет пустым
}

// Проверяем работу readBlocksFromDatabase: можем ли считывать данные правильно
TEST(ReadBlocksFromDatabaseTest, HandlesSimpleRead) {
    std::string testDbPath = "../../tests_common/db_for_unit_tests"; // Укажите путь к тестовым файлам, которые можно использовать

    std::vector<Block> blocks = readBlocksFromDatabase(testDbPath);

    std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) {
        return a.hash < b.hash;
    });

    // Проверяем, что файлы загружаются нормально
    ASSERT_FALSE(blocks.empty()); // Список не должен быть пустым
    EXPECT_EQ(blocks.size(), 3); // Проверяем, сколько блоков ожидаем
    EXPECT_EQ(blocks[0].hash, "block1_hash"); // Сравниваем с первым файлом .brik
    EXPECT_EQ(blocks[1].difficulty, 5); // Проверяем сложность у одного блока
}

// Проверяем, как readBlocksFromDatabase ведет себя без файлов
TEST(ReadBlocksFromDatabaseTest, HandlesNoFiles) {
    std::string emptyDbPath = "../../tests_common/empty_db_for_unit_tests";

    std::vector<Block> blocks = readBlocksFromDatabase(emptyDbPath);
    EXPECT_TRUE(blocks.empty()); // Список должен быть пуст, если файлов нет
}