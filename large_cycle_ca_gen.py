import random
import os
import json

RULE_CLASSES_TRANSITION = [
  {
    (51, 204, 60, 195): 0, 
    (85, 90, 165, 170): 1, 
    (102, 105, 150, 153): 2,
    (53, 58, 83, 92, 163, 172, 197, 202): 3,
    (54, 57, 99, 108, 147, 156, 198, 201): 4,
    (86, 89, 101, 106, 149, 154, 166, 169): 5 
  },
  {
    (15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240): 0
  },
  {
    (51, 204, 15, 240): 0, 
    (85, 105, 150, 170): 1,
    (90, 102, 153, 165): 2, 
    (23, 43, 77, 113, 142, 178, 212, 232): 3,
    (27, 39, 78, 114, 141, 177, 216, 228): 4,
    (86, 89, 101, 106, 149, 154, 166, 169): 5
  },
  {
    (60, 195): 0, 
    (90, 165): 3, 
    (105, 150): 4
  },
  {
    (51, 204): 0, 
    (85, 170): 1, 
    (102, 153): 2, 
    (86, 89, 90, 101, 105, 106, 149, 150, 154, 165, 166, 169): 5
  },
  {
    (15, 240): 0, 
    (105, 150): 3, 
    (90, 165): 4
  }
]

RULE_DEGREE_OF_DEPENDENCE = {
  0.25: (
    53, 58, 83, 92, 163, 172, 197, 202, 54, 57, 99, 108, 147, 156, 198, 201,
    23, 43, 77, 113, 142, 178, 212, 232, 27, 39, 78, 114, 141, 177, 216, 228,
  ), # weakly dependent
  0.5: (30, 45, 75, 120, 135, 180, 210, 225, 86, 89, 101, 106, 149, 154, 166, 169), # partially dependent
  1.0: (90, 165, 150, 105) # completely dependent
}

RULE_PARTITIONS = [
  {
    1.0: (90, 165, 105, 150), 
    0.25: (53, 58, 83, 92, 163, 172, 197, 202, 54, 57, 99, 108, 147, 156, 198, 201), 
    0.5: (86, 89, 101, 106, 149, 154, 166, 169)
  }, 
  {
    0.5: (30, 45, 75, 120, 135, 180, 210, 225), 
    1.0: (90, 105, 150, 165)
  },
  {
    1.0: (105, 150, 90, 165), 
    0.25: (23, 43, 77, 113, 142, 178, 212, 232, 27, 39, 78, 114, 141, 177, 216, 228), 
    0.5: (86, 89, 101, 106, 149, 154, 166, 169)
  }, 
  {
    1.0: (90, 165, 105, 150)
  },
  {
    0.5: (86, 89, 101, 106, 149, 154, 166, 169),
    1.0: (90, 105, 150, 165)
  },
  {
    1.0: (105, 150, 90, 165)
  }
]

FIRST_RULES = (5, 10, 9, 6)

FIRST_RULES_TRANSITION = {
  (5, 10): 1,
  (6, 9): 2
}

LAST_RULES_TRANSITION = {
  3: (20, 65),
  5: (5, 80)
}

# ONLY USE ONCE TO GENERATE RULE_PARTITIONS ABOVE
# def partition_classes():
#   partitions = [dict() for i in range(6)]
#   for i, rule_class in enumerate(RULE_CLASSES):
#     for rule in rule_class:
#       for key, value in RULE_DEGREE_OF_DEPENDENCE.items():
#         if rule in value:
#           partitions[i].setdefault(key, [])
#           partitions[i][key].append(rule)
#   return partitions

def generate_large_cycle_ca(length = 2):
  ca = []
  cur_rule = random.choice(FIRST_RULES)
  ca.append(cur_rule)
  next_rule_class = [value for key, value in FIRST_RULES_TRANSITION.items() if cur_rule in key][0]
  for i in range(1, length-1):
    random_var = random.random()
    if random_var < 0.35:
      if 0.5 in RULE_PARTITIONS[next_rule_class]:
        cur_rule = random.choice(RULE_PARTITIONS[next_rule_class][0.5])
      else:
        cur_rule = random.choice(RULE_PARTITIONS[next_rule_class][1.0])
    elif random_var < 0.75:
      cur_rule = random.choice(RULE_PARTITIONS[next_rule_class][1.0])
    else:
      if 0.25 in RULE_PARTITIONS[next_rule_class]:
        cur_rule = random.choice(RULE_PARTITIONS[next_rule_class][0.25])
      else:
        cur_rule = random.choice(RULE_PARTITIONS[next_rule_class][1.0])
    ca.append(cur_rule)
    next_rule_class = [value for key, value in RULE_CLASSES_TRANSITION[next_rule_class].items() if cur_rule in key][0]
  if next_rule_class in LAST_RULES_TRANSITION:
    ca.append(random.choice(LAST_RULES_TRANSITION[next_rule_class]))
    return tuple(ca)
  else:
    return None

if __name__ == "__main__":
  length = int(input("Enter required CA length: "))
  count = int(input("Enter number of large cycle length CAs needed (enter -1 to generate indefinitely): "))

  json_dump = {"CAs": set()}

  if count == -1:
    test_condition = True
  else:
    test_condition = len(json_dump["CAs"]) < count

  while test_condition:
    ca = generate_large_cycle_ca(length)
    if ca != None:
      json_dump["CAs"].add(ca)
      print("CAs generated till now: " + str(len(json_dump["CAs"])))
      print(ca)
    if count == -1:
      test_condition = True
    else:
      test_condition = len(json_dump["CAs"]) < count
  
  file_name = f"count_{count}_ca_length_{length}.json"
  
  if not os.path.exists("./large_cycle_CAs"):
      os.mkdir("./large_cycle_CAs")
  file = open(f"./large_cycle_CAs/{file_name}", "w")

  json_dump["CAs"] = list(json_dump["CAs"])
  json.dump(json_dump, file)

  file.close()
  
