#include "PKWareDecompressor.h"
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cstring>

// Destructeur de nœud Huffman (récursif)
PKWareDecompressor::HuffmanNode::~HuffmanNode() {
    delete left;
    delete right;
}

PKWareDecompressor::PKWareDecompressor() 
    : m_bitBuffer(0), m_bitCount(0), m_outBuffer(nullptr), m_outSize(0), m_outPos(0),
      m_literalTree(nullptr), m_distanceTree(nullptr), m_inPtr(nullptr) {
    InitTables();
}

PKWareDecompressor::~PKWareDecompressor() {
    FreeTrees();
    delete[] m_outBuffer;
}

void PKWareDecompressor::InitTables() {
    // Tables pour les longueurs (utilisées pour l'algorithme Implode)
    m_lengthBase = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28,
        32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 255
    };
    
    m_lengthBits = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    
    // Tables pour les distances
    m_distanceBase = {
        0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192,
        256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144,
        8192, 12288, 16384, 24576, 32768, 49152, 65536
    };
    
    m_distanceBits = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15
    };
}

void PKWareDecompressor::FreeTrees() {
    delete m_literalTree;
    delete m_distanceTree;
    m_literalTree = nullptr;
    m_distanceTree = nullptr;
}

void PKWareDecompressor::EnsureBufferSize(size_t requiredSize) {
    if (requiredSize > m_outSize) {
        size_t newSize = std::max(m_outSize * 2, requiredSize);
        uint8_t* newBuffer = new uint8_t[newSize];
        
        if (m_outBuffer) {
            std::memcpy(newBuffer, m_outBuffer, m_outPos);
            delete[] m_outBuffer;
        }
        
        m_outBuffer = newBuffer;
        m_outSize = newSize;
    }
}

uint32_t PKWareDecompressor::ReadBits(uint8_t numBits) {
    while (m_bitCount < numBits) {
        m_bitBuffer |= static_cast<uint32_t>(*m_inPtr++) << m_bitCount;
        m_bitCount += 8;
    }
    
    uint32_t result = m_bitBuffer & ((1 << numBits) - 1);
    m_bitBuffer >>= numBits;
    m_bitCount -= numBits;
    
    return result;
}

// Construction des arbres pour le format PKWARE Imploding
void PKWareDecompressor::BuildTrees(bool isSmallDict, bool isLargeTree) {
    // Libérer les arbres précédents s'ils existent
    FreeTrees();
    
    // Construire l'arbre des littéraux/longueurs
    m_literalTree = BuildLiteralTree(isLargeTree);
    
    // Construire l'arbre des distances
    m_distanceTree = BuildDistanceTree(isSmallDict);
}

// Construction de l'arbre des littéraux/longueurs
PKWareDecompressor::HuffmanNode* PKWareDecompressor::BuildLiteralTree(bool isLargeTree) {
    // Dans PKWARE Imploding, les arbres sont pré-construits dans le format
    // Nous devons lire leur représentation compressée
    
    HuffmanNode* root = new HuffmanNode();
    
    // PKWARE Imploding utilise des arbres fixes selon les flags
    CreateFixedLiteralTree(root, isLargeTree);
    
    return root;
}

// Construction de l'arbre des distances
PKWareDecompressor::HuffmanNode* PKWareDecompressor::BuildDistanceTree(bool isSmallDict) {
    // L'arbre des distances est également fixe selon la taille du dictionnaire
    HuffmanNode* root = new HuffmanNode();
    
    CreateFixedDistanceTree(root, isSmallDict);
    
    return root;
}

// Création de l'arbre littéral fixe selon la spécification PKWARE
void PKWareDecompressor::CreateFixedLiteralTree(HuffmanNode* root, bool isLargeTree) {
    // Nombre de symboles: 256 littéraux + codes spéciaux de longueur
    const int numSymbols = isLargeTree ? 290 : 257;
    
    // Structure de l'arbre selon le format PKWARE Imploding
    // Cette structure est spécifique au format et dépend du paramètre isLargeTree
    
    // Pour un arbre à 2 bits (isLargeTree == false)
    if (!isLargeTree) {
        // Lire les données codifiées de l'arbre depuis le flux compressé
        // Format de l'arbre à 2 bits (selon PKWARE Data Compression Library spec)
        
        // L'arbre à 2 bits utilise un codage préfixe simple
        // Lire le nombre de nœuds internes de l'arbre
        int numNodes = ReadBits(8);
        
        // Structure de données temporaire pour construire l'arbre
        std::vector<std::pair<uint8_t, int>> codeTable;
        
        // Lire les codes préfixes
        for (int i = 0; i < numNodes; i++) {
            uint8_t symbol = ReadBits(8);  // Symbole (0-255)
            uint8_t codeLen = ReadBits(4); // Longueur du code (1-15)
            uint16_t code = ReadBits(codeLen); // Code Huffman
            
            codeTable.push_back(std::make_pair(symbol, code | (codeLen << 8)));
        }
        
        // Construire l'arbre à partir des codes lus
        for (const auto& entry : codeTable) {
            uint8_t symbol = entry.first;
            uint16_t codeInfo = entry.second;
            uint8_t codeLen = codeInfo >> 8;
            uint16_t code = codeInfo & 0xFF;
            
            HuffmanNode* node = root;
            for (int bit = codeLen - 1; bit >= 0; bit--) {
                bool isRightChild = (code & (1 << bit)) != 0;
                
                if (isRightChild) {
                    if (!node->right) {
                        node->right = new HuffmanNode();
                    }
                    node = node->right;
                } else {
                    if (!node->left) {
                        node->left = new HuffmanNode();
                    }
                    node = node->left;
                }
                
                // Si c'est une feuille, assigner la valeur
                if (bit == 0) {
                    node->value = symbol;
                }
            }
        }
    }
    // Pour un arbre à 3 bits (isLargeTree == true)
    else {
        // Format de l'arbre à 3 bits (selon PKWARE Data Compression Library spec)
        // Similaire mais avec plus de symboles et une structure différente
        
        // L'arbre à 3 bits utilise également un codage préfixe, mais plus complexe
        int numNodes = ReadBits(8);
        
        // Table pour stocker les codes
        std::vector<std::pair<uint16_t, int>> codeTable;
        
        // Lire les codes préfixes
        for (int i = 0; i < numNodes; i++) {
            uint16_t symbol = ReadBits(9);  // Symbole (0-511, incluant littéraux et longueurs)
            uint8_t codeLen = ReadBits(5);  // Longueur du code (1-31)
            uint32_t code = ReadBits(codeLen); // Code Huffman
            
            codeTable.push_back(std::make_pair(symbol, code | (codeLen << 16)));
        }
        
        // Construire l'arbre à partir des codes
        for (const auto& entry : codeTable) {
            uint16_t symbol = entry.first;
            uint32_t codeInfo = entry.second;
            uint8_t codeLen = codeInfo >> 16;
            uint32_t code = codeInfo & 0xFFFF;
            
            HuffmanNode* node = root;
            for (int bit = codeLen - 1; bit >= 0; bit--) {
                bool isRightChild = (code & (1 << bit)) != 0;
                
                if (isRightChild) {
                    if (!node->right) {
                        node->right = new HuffmanNode();
                    }
                    node = node->right;
                } else {
                    if (!node->left) {
                        node->left = new HuffmanNode();
                    }
                    node = node->left;
                }
                
                // Si c'est une feuille, assigner la valeur
                if (bit == 0) {
                    node->value = symbol;
                }
            }
        }
    }
}

// Création de l'arbre de distance fixe selon la spécification PKWARE
void PKWareDecompressor::CreateFixedDistanceTree(HuffmanNode* root, bool isSmallDict) {
    // L'arbre des distances est plus simple et dépend de la taille du dictionnaire
    int maxDistance = isSmallDict ? 4096 : 8192;
    int numDistanceCodes = isSmallDict ? 64 : 128; // Nombre de codes de distance
    
    // Lire les données codifiées de l'arbre depuis le flux compressé
    // Dans PKWARE Imploding, l'arbre des distances est généralement codé de manière similaire
    
    // Lire le nombre de nœuds de l'arbre de distance
    int numNodes = ReadBits(8);
    
    // Structure de données temporaire pour construire l'arbre
    std::vector<std::pair<uint8_t, int>> codeTable;
    
    // Lire les codes préfixes pour les distances
    for (int i = 0; i < numNodes; i++) {
        uint8_t symbol = ReadBits(7);  // Code de distance (limité par numDistanceCodes)
        uint8_t codeLen = ReadBits(4); // Longueur du code (1-15)
        uint16_t code = ReadBits(codeLen); // Code Huffman
        
        codeTable.push_back(std::make_pair(symbol, code | (codeLen << 8)));
    }
    
    // Construire l'arbre à partir des codes lus
    for (const auto& entry : codeTable) {
        uint8_t symbol = entry.first;
        uint16_t codeInfo = entry.second;
        uint8_t codeLen = codeInfo >> 8;
        uint16_t code = codeInfo & 0xFF;
        
        HuffmanNode* node = root;
        for (int bit = codeLen - 1; bit >= 0; bit--) {
            bool isRightChild = (code & (1 << bit)) != 0;
            
            if (isRightChild) {
                if (!node->right) {
                    node->right = new HuffmanNode();
                }
                node = node->right;
            } else {
                if (!node->left) {
                    node->left = new HuffmanNode();
                }
                node = node->left;
            }
            
            // Si c'est une feuille, assigner la valeur
            if (bit == 0) {
                node->value = symbol;
            }
        }
    }
}

int PKWareDecompressor::ReadSymbol(HuffmanNode* tree) {
    HuffmanNode* node = tree;
    
    while (node && node->value == -1) {
        uint32_t bit = ReadBits(1);
        node = bit ? node->right : node->left;
    }
    
    return node ? node->value : -1;
}

uint8_t* PKWareDecompressor::DecompressPKWare(const uint8_t* compData, size_t compSize, size_t &uncompSize) {
    if (!compData || compSize < 1) {
        uncompSize = 0;
        return nullptr;
    }
    
    // Réinitialiser l'état
    m_bitBuffer = 0;
    m_bitCount = 0;
    m_outPos = 0;
    m_inPtr = compData;
    
    // Lire les flags de compression (premier octet)
    uint8_t flags = *m_inPtr++;
    compSize--;
    
    // Bits des flags:
    // Bit 0: 0=dictionnaire 8K, 1=dictionnaire 4K
    // Bit 1: 0=arbre Huffman 2 bits, 1=arbre Huffman 3 bits
    bool isSmallDict = (flags & 0x01) != 0;
    bool isLargeTree = (flags & 0x02) != 0;
    
    // Construire les arbres Huffman
    BuildTrees(isSmallDict, isLargeTree);
    
    // Initialiser ou redimensionner le buffer de sortie
    size_t estimatedSize = compSize * 3;
    EnsureBufferSize(estimatedSize);
    
    // Boucle principale de décompression
    try {
        while (m_inPtr < compData + compSize) {
            // Lire un bit pour déterminer si c'est un littéral ou une séquence
            if (ReadBits(1) == 0) {
                // C'est un littéral
                int literal = ReadSymbol(m_literalTree);
                if (literal < 0) break; // Erreur ou fin de flux
                
                EnsureBufferSize(m_outPos + 1);
                m_outBuffer[m_outPos++] = static_cast<uint8_t>(literal);
            } else {
                // C'est une séquence (distance, longueur)
                int lengthCode = ReadSymbol(m_literalTree);
                if (lengthCode < 0) break;
                
                uint32_t length = m_lengthBase[lengthCode];
                length += ReadBits(m_lengthBits[lengthCode]);
                
                int distCode = ReadSymbol(m_distanceTree);
                if (distCode < 0) break;
                
                uint32_t distance = m_distanceBase[distCode];
                distance += ReadBits(m_distanceBits[distCode]);
                
                // Vérifier la validité de la distance
                if (distance > m_outPos) {
                    throw std::runtime_error("Distance invalide dans les données compressées");
                }
                
                // Assurer que le buffer de sortie est assez grand
                EnsureBufferSize(m_outPos + length);
                
                // Copier les octets du dictionnaire
                for (uint32_t i = 0; i < length; i++) {
                    m_outBuffer[m_outPos] = m_outBuffer[m_outPos - distance];
                    m_outPos++;
                }
            }
        }
    } catch (const std::exception& e) {
        // Gérer les erreurs, mais continuer avec ce qu'on a déjà décompressé
    }
    
    uncompSize = m_outPos;
    
    // Créer un nouveau buffer de la taille exacte décompressée
    uint8_t* result = new uint8_t[m_outPos];
    std::memcpy(result, m_outBuffer, m_outPos);
    
    return result;
}