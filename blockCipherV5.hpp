#include "fsa.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <bitset>
#include <thread>

#define BLOCKS_PER_GROUP 16
#define CA_COUNT 1000
#define KEY_COUNT 1
#define IS_CONST_STEPS true
#define CONST_STEPS 64

using namespace std;
const string FILE_NAME = "../large_cycle_CAs/count_" + to_string(CA_COUNT) + "_ca_length_" + to_string(BLOCK_SIZE) + ".json";

int keys[KEY_COUNT][BLOCK_SIZE + 1];
int linearCA[BLOCK_SIZE];

void getRuleVectorsFromFile(string fileName)
{

  int ruleVectors[CA_COUNT][BLOCK_SIZE + 1];
  ifstream file(fileName);
  if (file.fail())
  {
    cout << "Error opening CA file";
    exit(1);
  }
  string word;
  file >> word;
  random_device dev;
  mt19937 rng(dev());
  int keyCount = 0, rulePos = 0;
  while (file >> word)
  {
    string num;
    for (char c : word)
    {
      if (isdigit(c))
      {
        num += c;
      }
    }
    int numVal = stoi(num);
    ruleVectors[keyCount][rulePos++] = numVal;
    if (word[word.size() - 2] == ']' && rulePos != 0)
    {
      keyCount++;
      rulePos = 0;
    }
  }

  file.close();

  uniform_int_distribution<mt19937::result_type> fwdStepsDist(1, 1 << (8)); // distribution in range [1, 2^(BLOCK_SIZE-1)]
  uniform_int_distribution<mt19937::result_type> dist2(0, CA_COUNT - 1);
  for (int i = 0; i < KEY_COUNT; ++i)
  {
    int randomKeyIdx = dist2(rng);
    for (int j = 0; j < BLOCK_SIZE; ++j)
    {
      keys[i][j] = ruleVectors[randomKeyIdx][j];
    }

    if (IS_CONST_STEPS)
    {
      keys[i][BLOCK_SIZE] = CONST_STEPS;
    }
    else
    {
      keys[i][BLOCK_SIZE] = fwdStepsDist(rng);
    }
  }

  // initialize linear CA
  for (int i = 0; i < BLOCK_SIZE; ++i)
  {
    linearCA[i] = 153;
  }
}

bitset<BLOCK_SIZE> blockEncrypt(bitset<BLOCK_SIZE> plaintextBlock)
{
  bitset<BLOCK_SIZE> ciphertextBlock = plaintextBlock;

  // non linear step
  for (int i = 0; i < KEY_COUNT; ++i)
  {
    for (int step = 0; step < keys[i][BLOCK_SIZE]; ++step)
    {
      ciphertextBlock = nextConf(ciphertextBlock, keys[i]);
    }
  }

  // linear step
  for (int i = 0; i < BLOCK_SIZE; ++i)
  {
    ciphertextBlock ^= (ciphertextBlock << 1);
    ciphertextBlock = ~ciphertextBlock;
  }

  return ciphertextBlock;
}

bitset<BLOCK_SIZE> blockDecrypt(bitset<BLOCK_SIZE> plaintextBlock)
{
  bitset<BLOCK_SIZE> ciphertextBlock = plaintextBlock;

  // linear step
  for (int i = 0; i < BLOCK_SIZE; ++i)
  {
    ciphertextBlock ^= (ciphertextBlock << 1);
    ciphertextBlock = ~ciphertextBlock;
  }

  // non linear step
  for (int i = KEY_COUNT - 1; i >= 0; --i)
  {
    for (int step = 0; step < keys[i][BLOCK_SIZE]; ++step)
    {
      ciphertextBlock = prevConf(ciphertextBlock, keys[i]);
    }
  }

  return ciphertextBlock;
}

void threadEncrypt(vector<bitset<BLOCK_SIZE>> plaintext, vector<bitset<BLOCK_SIZE>> &ciphertext, int group)
{
  // Initialization Vector initialisation
  bitset<BLOCK_SIZE> IV;
  random_device dev;
  mt19937 rng(dev());
  uniform_int_distribution<mt19937::result_type> dist1(0, 1);
  for (int i = 0; i < BLOCK_SIZE; ++i)
  {
    IV[i] = dist1(rng);
  }

  size_t CTidx = group * (BLOCKS_PER_GROUP + 1);
  size_t PTidx = CTidx - group;
  ciphertext[CTidx++] = IV;
  while (CTidx <= group * (BLOCKS_PER_GROUP + 1) + BLOCKS_PER_GROUP && CTidx < ciphertext.size())
  {
    ciphertext[CTidx] = blockEncrypt(ciphertext[CTidx - 1] ^ plaintext[PTidx]);
    CTidx++;
    PTidx++;
  }
}

vector<bitset<BLOCK_SIZE>> encrypt(vector<bitset<BLOCK_SIZE>> plaintextBlocks)
{
  // divide blocks into groups for concurrent encryption
  size_t groups = ceil(plaintextBlocks.size() / (float)BLOCKS_PER_GROUP);
  size_t ciphertextBlockCount = plaintextBlocks.size() + groups;
  vector<bitset<BLOCK_SIZE>> ciphertext(ciphertextBlockCount);
  vector<thread> threads;
  for (int group = 0; group < groups; ++group)
  {
    threads.emplace_back(thread(threadEncrypt, plaintextBlocks, ref(ciphertext), group));
  }
  for (auto &thread : threads)
  {
    thread.join();
  }
  return ciphertext;
}

vector<bitset<BLOCK_SIZE>> encrypt(string plaintext)
{
  // convert plaintext string into list of binary blocks
  vector<bitset<BLOCK_SIZE>> plaintextBlocks;
  bitset<BLOCK_SIZE> plaintextBlock;
  int idx = BLOCK_SIZE - 1;
  for (int i = 0; i < plaintext.size(); ++i)
  {
    for (int j = 7; j >= 0; --j)
    {
      plaintextBlock[idx--] = (plaintext[i] >> j) & 1;
    }
    if (idx < 0)
    {
      plaintextBlocks.push_back(plaintextBlock);
      idx = BLOCK_SIZE - 1;
    }
  }
  if (idx >= 0 && idx < BLOCK_SIZE - 1)
  {
    while (idx >= 0)
    {
      plaintextBlock.reset(idx--);
    }
    plaintextBlocks.push_back(plaintextBlock);
  }

  // divide blocks into groups for concurrent encryption
  size_t groups = ceil(plaintextBlocks.size() / (float)BLOCKS_PER_GROUP);
  size_t ciphertextBlockCount = plaintextBlocks.size() + groups;
  vector<bitset<BLOCK_SIZE>> ciphertext(ciphertextBlockCount);
  vector<thread> threads;
  for (int group = 0; group < groups; ++group)
  {
    threads.emplace_back(thread(threadEncrypt, plaintextBlocks, ref(ciphertext), group));
  }
  for (auto &thread : threads)
  {
    thread.join();
  }
  return ciphertext;
}

void threadDecrypt(vector<bitset<BLOCK_SIZE>> ciphertext, vector<bitset<BLOCK_SIZE>> &plaintextBinary, int group)
{
  for (int i = group * (BLOCKS_PER_GROUP + 1) + 1; i <= group * (BLOCKS_PER_GROUP + 1) + BLOCKS_PER_GROUP && i < ciphertext.size(); ++i)
  {
    plaintextBinary.push_back(ciphertext[i - 1] ^ blockDecrypt(ciphertext[i]));
  }
}

string decrypt(vector<bitset<BLOCK_SIZE>> ciphertextBlocks)
{
  size_t groups = ceil(ciphertextBlocks.size() / (float)(BLOCKS_PER_GROUP + 1));
  vector<bitset<BLOCK_SIZE>> plaintextBinary;

  vector<thread> threads;
  for (int group = 0; group < groups; ++group)
  {
    threads.emplace_back(thread(threadDecrypt, ciphertextBlocks, ref(plaintextBinary), group));
  }
  for (auto &thread : threads)
  {
    thread.join();
  }

  string plaintext;
  char c = 0;
  for (auto block : plaintextBinary)
  {
    for (int j = 0; j < BLOCK_SIZE; ++j)
    {
      if (j % 8 == 0 && c)
      {
        plaintext.push_back(c);
        c = 0;
      }
      c |= (block[REV_IDX(j, BLOCK_SIZE)] << REV_IDX(j, BLOCK_SIZE) % 8);
    }
  }
  plaintext.push_back(c);
  return plaintext;
}
