#include <gtest/gtest.h>
#include "commons/ByteStream.h"
#include <vector>

class ByteStreamTest : public ::testing::Test {
protected:
    std::vector<uint8_t> testData;
    
    void SetUp() override {
        testData = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    }
};

TEST_F(ByteStreamTest, ReadByte_ReturnsCorrectValue_AndAdvancesPosition) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    EXPECT_EQ(0x01, stream.ReadByte());
    EXPECT_EQ(1, stream.GetCurrentPosition());
    EXPECT_EQ(0x02, stream.ReadByte());
    EXPECT_EQ(2, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadByte_BeyondEnd_ReturnsZero_AndDoesNotAdvance) {
    uint8_t data[] = {0x01};
    ByteStream stream(data, 1);
    
    stream.ReadByte();
    EXPECT_EQ(1, stream.GetCurrentPosition());
    EXPECT_EQ(0, stream.ReadByte());
    EXPECT_EQ(1, stream.GetCurrentPosition()); // Position ne change pas au-delà de la fin
}

TEST_F(ByteStreamTest, ReadUShort_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    ByteStream stream(data, 4);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    uint16_t expected = 0x0201; // Little endian
    EXPECT_EQ(expected, stream.ReadUShort());
    EXPECT_EQ(2, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadShort_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0xFF, 0xFF, 0x01, 0x02}; // -1 en little endian
    ByteStream stream(data, 4);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    EXPECT_EQ(-1, stream.ReadShort());
    EXPECT_EQ(2, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadUInt32LE_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    ByteStream stream(data, 6);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    uint32_t expected = 0x04030201;
    EXPECT_EQ(expected, stream.ReadUInt32LE());
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadInt32LE_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x02}; // -1
    ByteStream stream(data, 6);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    EXPECT_EQ(-1, stream.ReadInt32LE());
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadUInt32BE_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    ByteStream stream(data, 6);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    uint32_t expected = 0x01020304;
    EXPECT_EQ(expected, stream.ReadUInt32BE());
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadUShortBE_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    ByteStream stream(data, 4);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    uint16_t expected = 0x0102;
    EXPECT_EQ(expected, stream.ReadUShortBE());
    EXPECT_EQ(2, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadString_ReturnsCorrectString_AndAdvancesPosition) {
    uint8_t data[] = {'H', 'e', 'l', 'l', 'o', 0x00, 'X', 'Y'};
    ByteStream stream(data, 8);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    std::string result = stream.ReadString(6);
    EXPECT_EQ("Hello", result);
    EXPECT_EQ(6, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ByteStreamTest_ReadString_MaxSizeReached_StripNull_Test) {
    uint8_t data[] = {'T', 'e', 's', 't', 0x00, 'X', 'Y', 'Z'};
    ByteStream stream(data, 8);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    std::string result = stream.ReadString(6);
    EXPECT_EQ("TestX", result);
    EXPECT_EQ(6, stream.GetCurrentPosition()); // Lit jusqu'à la taille spécifiée
}

TEST_F(ByteStreamTest, ReadStringNoSize_ReturnsCorrectString_AndAdvancesPosition) {
    uint8_t data[] = {'T', 'e', 's', 't', 0x00, 0xFF, 0xAA};
    ByteStream stream(data, 7);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    std::string result = stream.ReadStringNoSize(6);
    EXPECT_EQ("Test", result);
    EXPECT_EQ(5, stream.GetCurrentPosition()); // S'arrête au null + 1
}

TEST_F(ByteStreamTest, ReadStringNoSize_StopsAtNull_AndAdvancesPosition) {
    uint8_t data[] = {'A', 'B', 0x00, 'C', 'D', 'E'};
    ByteStream stream(data, 6);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    std::string result = stream.ReadStringNoSize(5);
    EXPECT_EQ("AB", result);
    EXPECT_EQ(3, stream.GetCurrentPosition()); // 'A', 'B', 0x00
}

TEST_F(ByteStreamTest, MoveForward_MovesPosition_Correctly) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    stream.MoveForward(3);
    EXPECT_EQ(3, stream.GetCurrentPosition());
    EXPECT_EQ(0x04, stream.ReadByte());
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, MoveForward_BeyondEnd_ClampedToSize) {
    uint8_t data[] = {0x01, 0x02};
    ByteStream stream(data, 2);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    stream.MoveForward(100);
    EXPECT_EQ(1, stream.GetCurrentPosition()); // Limité à la taille - 1
    EXPECT_EQ(0x02, stream.ReadByte()); // Devrait retourner 0
    EXPECT_EQ(2, stream.GetCurrentPosition()); // Position ne change pas
}

TEST_F(ByteStreamTest, PeekByte_DoesNotMovePosition) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    uint8_t peeked = stream.PeekByte();
    EXPECT_EQ(0x02, peeked);
    EXPECT_EQ(0, stream.GetCurrentPosition()); // Position inchangée
    
    uint8_t current = stream.CurrentByte();
    EXPECT_EQ(0x01, current);
    EXPECT_EQ(0, stream.GetCurrentPosition()); // Position toujours inchangée
    
    EXPECT_EQ(0x01, stream.ReadByte());
    EXPECT_EQ(1, stream.GetCurrentPosition()); // Maintenant la position avance
}

TEST_F(ByteStreamTest, CurrentByte_DoesNotMovePosition) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    EXPECT_EQ(0x01, stream.CurrentByte());
    EXPECT_EQ(0, stream.GetCurrentPosition()); // Position inchangée
    
    stream.ReadByte();
    EXPECT_EQ(1, stream.GetCurrentPosition());
    EXPECT_EQ(0x02, stream.CurrentByte());
    EXPECT_EQ(1, stream.GetCurrentPosition()); // Position toujours inchangée
}

TEST_F(ByteStreamTest, ReadBytes_Vector_ReturnsCorrectData_AndAdvancesPosition) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    auto result = stream.ReadBytes(4);
    
    ASSERT_EQ(4, result.size());
    EXPECT_EQ(0x01, result[0]);
    EXPECT_EQ(0x02, result[1]);
    EXPECT_EQ(0x03, result[2]);
    EXPECT_EQ(0x04, result[3]);
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadBytes_Buffer_ReadsCorrectData_AndAdvancesPosition) {
    ByteStream stream(testData.data(), testData.size());
    uint8_t buffer[4];
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    stream.ReadBytes(buffer, 4);
    
    EXPECT_EQ(0x01, buffer[0]);
    EXPECT_EQ(0x02, buffer[1]);
    EXPECT_EQ(0x03, buffer[2]);
    EXPECT_EQ(0x04, buffer[3]);
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadBytes_BeyondEnd_ReadsAvailableBytes_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02};
    ByteStream stream(data, 2);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    auto result = stream.ReadBytes(10);
    
    EXPECT_EQ(2, result.size());
    EXPECT_EQ(2, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, ReadInt24LE_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x00, 0xFF};
    ByteStream stream(data, 5);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    int32_t expected = 0x030201;
    EXPECT_EQ(expected, stream.ReadInt24LE());
    EXPECT_EQ(4, stream.GetCurrentPosition()); // ReadInt24LE lit 4 bytes
}

TEST_F(ByteStreamTest, ReadInt24LEByte3_ReturnsCorrectValue_AndAdvancesPosition) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0xFF};
    ByteStream stream(data, 4);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    int32_t expected = 0x030201;
    EXPECT_EQ(expected, stream.ReadInt24LEByte3());
    EXPECT_EQ(3, stream.GetCurrentPosition()); // ReadInt24LEByte3 lit 3 bytes
}

TEST_F(ByteStreamTest, ReadInt24LE_NegativeValue_HandlesSignExtension_AndAdvancesPosition) {
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0x00, 0xAA};
    ByteStream stream(data, 5);
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    int32_t result = stream.ReadInt24LE();
    EXPECT_EQ(-1, result);
    EXPECT_EQ(4, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, Set_ResetsStream_AndPosition) {
    ByteStream stream;
    
    stream.Set(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    EXPECT_EQ(0x01, stream.ReadByte());
    EXPECT_EQ(1, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, CopyConstructor_CopiesCursor_AndPosition) {
    ByteStream original(testData.data(), testData.size());
    original.ReadByte(); // Avance d'un byte
    
    EXPECT_EQ(1, original.GetCurrentPosition());
    ByteStream copy(original);
    
    EXPECT_EQ(0x02, copy.ReadByte());
    EXPECT_EQ(2, copy.GetCurrentPosition());
}

TEST_F(ByteStreamTest, GetPosition_ReturnsCurrentPointer) {
    ByteStream stream(testData.data(), testData.size());
    
    uint8_t* initial = stream.GetPosition();
    EXPECT_EQ(0, stream.GetCurrentPosition());
    
    stream.ReadByte();
    uint8_t* after = stream.GetPosition();
    
    EXPECT_EQ(initial + 1, after);
    EXPECT_EQ(1, stream.GetCurrentPosition());
}

TEST_F(ByteStreamTest, MultipleOperations_MaintainCorrectPosition) {
    ByteStream stream(testData.data(), testData.size());
    
    EXPECT_EQ(0, stream.GetCurrentPosition());
    stream.ReadByte(); // Position = 1
    EXPECT_EQ(1, stream.GetCurrentPosition());
    
    stream.ReadUShort(); // Position = 3
    EXPECT_EQ(3, stream.GetCurrentPosition());
    
    stream.MoveForward(2); // Position = 5
    EXPECT_EQ(5, stream.GetCurrentPosition());
    
    stream.ReadByte(); // Position = 6
    EXPECT_EQ(6, stream.GetCurrentPosition());
    
    EXPECT_EQ(0x07, stream.CurrentByte()); // Position inchangée
    EXPECT_EQ(6, stream.GetCurrentPosition());
}