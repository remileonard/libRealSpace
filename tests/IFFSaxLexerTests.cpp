#include <gtest/gtest.h>
#include "../src/commons/IFFSaxLexer.h"
#include <vector>
#include <cstring>

class IFFSaxLexerTest : public ::testing::Test {
protected:
    IFFSaxLexer* lexer;

    void SetUp() override {
        lexer = new IFFSaxLexer();
    }

    void TearDown() override {
        delete lexer;
    }

    // Helper pour créer un chunk IFF simple
    std::vector<uint8_t> CreateSimpleChunk(const char* chunkType, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result;
        
        // Ajouter le type de chunk (4 octets)
        for (int i = 0; i < 4; i++) {
            result.push_back(chunkType[i]);
        }
        
        // Ajouter la taille (big-endian)
        uint32_t size = data.size();
        result.push_back((size >> 24) & 0xFF);
        result.push_back((size >> 16) & 0xFF);
        result.push_back((size >> 8) & 0xFF);
        result.push_back(size & 0xFF);
        
        // Ajouter les données
        result.insert(result.end(), data.begin(), data.end());
        
        // Padding si nécessaire
        if (data.size() % 2 != 0) {
            result.push_back(0);
        }
        
        return result;
    }

    // Helper pour créer un chunk FORM
    std::vector<uint8_t> CreateFormChunk(const char* formType, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> result;
        
        // "FORM"
        result.push_back('F');
        result.push_back('O');
        result.push_back('R');
        result.push_back('M');
        
        // Taille (4 octets form type + données)
        uint32_t size = 4 + data.size();
        result.push_back((size >> 24) & 0xFF);
        result.push_back((size >> 16) & 0xFF);
        result.push_back((size >> 8) & 0xFF);
        result.push_back(size & 0xFF);
        
        // Type de FORM (4 octets)
        for (int i = 0; i < 4; i++) {
            result.push_back(formType[i]);
        }
        
        // Données
        result.insert(result.end(), data.begin(), data.end());
        
        // Padding si nécessaire
        if (size % 2 != 0) {
            result.push_back(0);
        }
        
        return result;
    }
};

// Test du constructeur
TEST_F(IFFSaxLexerTest, ConstructorCreatesValidObject) {
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    
    // Le constructeur devrait créer un objet valide qui peut être utilisé
    EXPECT_NO_THROW({
        std::vector<uint8_t> dummyData = {0x00};
        lexer->InitFromRAM(dummyData.data(), dummyData.size(), events);
    });
}

// Test InitFromRAM avec un chunk simple
TEST_F(IFFSaxLexerTest, InitFromRAMWithSimpleChunk) {
    bool eventCalled = false;
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0x04};
    std::vector<uint8_t> chunkData = CreateSimpleChunk("TEST", testData);
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["TEST"] = [&eventCalled, &testData](uint8_t* data, size_t size) {
        eventCalled = true;
        EXPECT_EQ(size, testData.size());
        EXPECT_EQ(memcmp(data, testData.data(), size), 0);
    };
    
    bool result = lexer->InitFromRAM(chunkData.data(), chunkData.size(), events);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(eventCalled);
}

// Test InitFromRAM avec un chunk FORM
TEST_F(IFFSaxLexerTest, InitFromRAMWithFormChunk) {
    bool eventCalled = false;
    std::vector<uint8_t> innerData = {0xAA, 0xBB, 0xCC, 0xDD};
    std::vector<uint8_t> chunkData = CreateFormChunk("MYFT", innerData);
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["MYFT"] = [&eventCalled](uint8_t* data, size_t size) {
        eventCalled = true;
        EXPECT_GT(size, 0);
    };
    
    bool result = lexer->InitFromRAM(chunkData.data(), chunkData.size(), events);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(eventCalled);
}

// Test avec plusieurs chunks
TEST_F(IFFSaxLexerTest, InitFromRAMWithMultipleChunks) {
    int callCount = 0;
    
    std::vector<uint8_t> data1 = {0x01, 0x02};
    std::vector<uint8_t> data2 = {0x03, 0x04, 0x05};
    
    std::vector<uint8_t> chunk1 = CreateSimpleChunk("CHK1", data1);
    std::vector<uint8_t> chunk2 = CreateSimpleChunk("CHK2", data2);
    
    std::vector<uint8_t> combinedData;
    combinedData.insert(combinedData.end(), chunk1.begin(), chunk1.end());
    combinedData.insert(combinedData.end(), chunk2.begin(), chunk2.end());
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["CHK1"] = [&callCount](uint8_t* data, size_t size) { callCount++; };
    events["CHK2"] = [&callCount](uint8_t* data, size_t size) { callCount++; };
    
    lexer->InitFromRAM(combinedData.data(), combinedData.size(), events);
    
    EXPECT_EQ(callCount, 2);
}

// Test avec un chunk non géré
TEST_F(IFFSaxLexerTest, InitFromRAMWithUnhandledChunk) {
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03};
    std::vector<uint8_t> chunkData = CreateSimpleChunk("UNKN", testData);
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    // Aucun handler pour "UNKN"
    
    // Ne devrait pas crasher
    EXPECT_NO_THROW({
        lexer->InitFromRAM(chunkData.data(), chunkData.size(), events);
    });
}

// Test avec un chunk de taille 0
TEST_F(IFFSaxLexerTest, InitFromRAMWithZeroSizeChunk) {
    bool eventCalled = false;
    std::vector<uint8_t> emptyData;
    std::vector<uint8_t> chunkData = CreateSimpleChunk("EMPT", emptyData);
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["EMPT"] = [&eventCalled](uint8_t* data, size_t size) {
        eventCalled = true;
        EXPECT_EQ(data, nullptr);
        EXPECT_EQ(size, 0);
    };
    
    lexer->InitFromRAM(chunkData.data(), chunkData.size(), events);
    
    EXPECT_TRUE(eventCalled);
}

// Test avec des données vides
TEST_F(IFFSaxLexerTest, InitFromRAMWithEmptyData) {
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    
    uint8_t emptyData[1] = {0};
    bool result = lexer->InitFromRAM(emptyData, 0, events);
    
    EXPECT_TRUE(result);
}

// Test InitFromFile avec un fichier inexistant
TEST_F(IFFSaxLexerTest, InitFromFileWithNonExistentFile) {
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    
    bool result = lexer->InitFromFile("nonexistent_file_xyz.iff", events);
    
    EXPECT_FALSE(result);
}

// Test du padding impair
TEST_F(IFFSaxLexerTest, HandlesOddSizedChunksWithPadding) {
    bool eventCalled = false;
    std::vector<uint8_t> oddData = {0x01, 0x02, 0x03}; // Taille impaire
    std::vector<uint8_t> chunkData = CreateSimpleChunk("ODDS", oddData);
    
    std::unordered_map<std::string, std::function<void(uint8_t*, size_t)>> events;
    events["ODDS"] = [&eventCalled, &oddData](uint8_t* data, size_t size) {
        eventCalled = true;
        // La taille devrait inclure le padding
        EXPECT_GE(size, oddData.size());
    };
    
    lexer->InitFromRAM(chunkData.data(), chunkData.size(), events);
    
    EXPECT_TRUE(eventCalled);
}