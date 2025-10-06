#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

// Format PKWARE Data Compression Library Imploding (ancien IBM TERSE)
class PKWareDecompressor {
private:
    // Tables pour la décompression
    std::vector<uint16_t> m_lengthBase;
    std::vector<uint8_t> m_lengthBits;
    std::vector<uint32_t> m_distanceBase;
    std::vector<uint8_t> m_distanceBits;
    
    // État interne pour la décompression
    uint32_t m_bitBuffer;
    int m_bitCount;
    uint8_t* m_outBuffer;
    size_t m_outSize;
    size_t m_outPos;
    const uint8_t* m_inPtr;
    
    // Structure de l'arbre Huffman
    struct HuffmanNode {
        int value;
        HuffmanNode* left;
        HuffmanNode* right;
        
        HuffmanNode() : value(-1), left(nullptr), right(nullptr) {}
        ~HuffmanNode();
    };
    
    HuffmanNode* m_literalTree;
    HuffmanNode* m_distanceTree;
    
    // Méthodes privées
    void InitTables();
    void BuildTrees(bool isSmallDict, bool isLargeTree);
    void FreeTrees();
    uint32_t ReadBits(uint8_t numBits);
    int ReadSymbol(HuffmanNode* tree);
    void EnsureBufferSize(size_t requiredSize);
    
    // Méthodes spécifiques à la construction des arbres
    HuffmanNode* BuildLiteralTree(bool isLargeTree);
    HuffmanNode* BuildDistanceTree(bool isSmallDict);
    void CreateFixedLiteralTree(HuffmanNode* root, bool isLargeTree);
    void CreateFixedDistanceTree(HuffmanNode* root, bool isSmallDict);
    
public:
    PKWareDecompressor();
    ~PKWareDecompressor();
    
    // Décompresse les données en utilisant l'algorithme PKWARE Imploding
    uint8_t* DecompressPKWare(const uint8_t* compData, size_t compSize, size_t &uncompSize);
};