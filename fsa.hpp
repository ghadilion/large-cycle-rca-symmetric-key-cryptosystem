#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <bitset>

#define BLOCK_SIZE 128
#define LEFT_BIT(num) (num >> 2 & 1)
#define MID_BIT(num) (num >> 1 & 1)
#define RIGHT_BIT(num) (num & 1)
#define HOOD_GEN(a, b, c) ((a << 2) | (b << 1) | c)
#define REV_IDX(idx, size) (size - idx - 1)
#define PREV_BIT 0
#define PREV_PREV_BIT 1
#define INEQUALITY 0
#define EQUALITY 1

using namespace std;

bitset<BLOCK_SIZE> nextConf(bitset<BLOCK_SIZE> conf, int ruleVector[])
{
  bitset<BLOCK_SIZE> nextConf;
  bitset<3> neighbourhood;
  for (int pos = 0; pos < BLOCK_SIZE; ++pos)
  {
    bool leftBit = (pos == BLOCK_SIZE - 1 ? 0 : conf[pos + 1]), rightBit = (pos == 0 ? 0 : conf[pos - 1]);
    neighbourhood = HOOD_GEN(leftBit, conf[pos], rightBit);
    nextConf[pos] = (ruleVector[BLOCK_SIZE - pos - 1] >> neighbourhood.to_ulong()) & 1;
  }
  return nextConf;
}

bitset<BLOCK_SIZE> prevConf(bitset<BLOCK_SIZE> conf, int ruleVector[])
{
  /*
    prevConf: stores previous configuration (to be returned)
    isSet: 1 if corresponding prevConf bit is set
    isDependent: 1 if corresponding prevConf bit is dependent on another bit
    dependentOn: 0 if corresponding prevConf bit is dependent on bit just to the left of it,
                 1 if it is dependent on the bit 2 positions left of itself
    dependenceType: 0 if the type of dependence is inequality, 1 if type is equality
  */
  bitset<BLOCK_SIZE> prevConf, isSet, isDependent, dependentOn, dependenceType;

  // resolve bits and discover dependencies (left-to-right pass)
  for (int pos = BLOCK_SIZE - 1; pos >= 0; --pos)
  {

    // candidate rmt generation (narrow down)
    bool isRightBitSet = (pos == 0); // right bit is only known during null boundary
    bool isLeftBitSet = (pos == BLOCK_SIZE - 1 || isSet[pos + 1]);
    bool leftBit = (pos == BLOCK_SIZE - 1 ? 0 : prevConf[pos + 1]);

    // resolution
    int candidateRmtAND = 7, candidateRmtNOR = 0;
    bool dependence[4] = {1, 1, 1, 1};

    /*
    dependence values at indexes:

    0: MID_BIT == RIGHT_BIT   (mid and right bits are 1)
    1: LEFT_BIT == RIGHT_BIT  (left and right bits are 1)
    2: MID_BIT != RIGHT_BIT   (bitwise not of 3)
    3: LEFT_BIT != RIGHT_BIT  (bitwise not of 5)
    */

    // narrow down loop
    for (int i = (isLeftBitSet ? leftBit : 0); i <= (isLeftBitSet ? leftBit : 1); ++i)
    {
      for (int j = (isSet[pos] ? prevConf[pos] : 0); j <= (isSet[pos] ? prevConf[pos] : 1); ++j)
      {
        for (int k = 0; k <= (isRightBitSet ? 0 : 1); ++k)
        {
          int rmt = HOOD_GEN(i, j, k);
          bool isRmtObeysDependence = (dependenceType[pos] == INEQUALITY && i != j) || (dependenceType[pos] == EQUALITY && i == j);
          bool isCandidateRmt = !isDependent[pos] || dependentOn[pos] == PREV_PREV_BIT || (dependentOn[pos] == PREV_BIT && isRmtObeysDependence);
          bool isCorrectBit = (ruleVector[REV_IDX(pos, BLOCK_SIZE)] >> rmt & 1) == conf[pos];
          if (isCandidateRmt && isCorrectBit)
          {
            // capture set and unset bit information from rmt
            candidateRmtAND &= rmt;
            candidateRmtNOR |= rmt;

            // store dependence information between pairs of positions (either equal or unequal)
            dependence[0] &= MID_BIT(rmt) == RIGHT_BIT(rmt);
            dependence[1] &= LEFT_BIT(rmt) == RIGHT_BIT(rmt);
            dependence[2] &= MID_BIT(rmt) != RIGHT_BIT(rmt);
            dependence[3] &= LEFT_BIT(rmt) != RIGHT_BIT(rmt);
          }
        }
      }
    }

    candidateRmtNOR = ~candidateRmtNOR;

    // dependency resolution
    if (pos + 1 < BLOCK_SIZE && pos - 1 >= 0 && (dependence[1] || dependence[3]) && !isSet[pos + 1] && !isSet[pos - 1])
    {
      isDependent[pos - 1] = 1;
      dependentOn[pos - 1] = PREV_PREV_BIT;
      dependenceType[pos - 1] = (dependence[1] ? EQUALITY : INEQUALITY);
    }

    if (pos - 1 >= 0 && (dependence[0] || dependence[2]) && !isSet[pos] && !isSet[pos - 1])
    {
      isDependent[pos - 1] = 1;
      dependentOn[pos - 1] = PREV_BIT;
      dependenceType[pos - 1] = (dependence[0] ? EQUALITY : INEQUALITY);
    }

    // bit resolution
    if (pos != BLOCK_SIZE - 1 && LEFT_BIT(candidateRmtAND))
    {
      isSet[pos + 1] = prevConf[pos + 1] = 1;
    }
    if (MID_BIT(candidateRmtAND))
    {
      isSet[pos] = prevConf[pos] = 1;
    }
    if (pos != 0 && RIGHT_BIT(candidateRmtAND))
    {
      isSet[pos - 1] = prevConf[pos - 1] = 1;
    }

    if (pos != BLOCK_SIZE - 1 && LEFT_BIT(candidateRmtNOR))
    {
      isSet[pos + 1] = 1;
      prevConf[pos + 1] = 0;
    }
    if (MID_BIT(candidateRmtNOR))
    {
      isSet[pos] = 1;
      prevConf[pos] = 0;
    }
    if (pos != 0 && RIGHT_BIT(candidateRmtNOR))
    {
      isSet[pos - 1] = 1;
      prevConf[pos - 1] = 0;
    }
  }

  // use dependencies to resolve missing bits (right-to-left pass)
  for (int i = 0; i < BLOCK_SIZE; ++i)
  {

    if (i >= BLOCK_SIZE || i < 0 || !isSet[i])
    {
      continue;
    }
    for (int j = i; isDependent[j]; j += dependentOn[j] + 1)
    {
      isSet[j + dependentOn[j] + 1] = 1;
      prevConf[j + dependentOn[j] + 1] = (dependenceType[j] == EQUALITY ? prevConf[j] : !prevConf[j]);
      isDependent[j] = 0;
    }
  }

  return prevConf;
}

// void printEdges (unordered_map<string, string> edges) {
//   cout << endl;
//   for (auto it : edges) {
//     cout << it.first << " -> " << it.second << endl;
//   }
// }

// unordered_map<string, string> buildFsa (vector<int> ruleVector, bool isPeriodic = false) {
//   int size = ruleVector.size();
//   unordered_map<string, string> edges;
//   for(uint64_t confValue = 0; confValue < (1 << size); ++confValue) {
//     string conf = bitset<64>(confValue).to_string();
//     edges[conf] = nextConf(conf, ruleVector, isPeriodic);
//   }
//   return edges;
// }