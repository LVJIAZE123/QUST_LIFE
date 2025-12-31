#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <vector>

// 中文注释：本文件实现了一个简易的文字冒险游戏，围绕学习、健康、心情与财富展开。

struct Person {
    std::string name;
    int age = 20;
    int day = 1;
    int knowledge = 0;
    int health = 0;
    int energy = 0;
    int mood = 0;
    int money = 0;
    bool keepPlaying = true;
    bool success = false;
};

struct Item {
    std::string name;
    std::string description;
    int cost = 0;
    std::function<void(Person&)> effect;
};

struct Action {
    std::string name;
    std::string description;
    std::function<void(Person&, std::vector<Item>&)> execute;
};

struct RandomEvent {
    std::string description;
    std::function<bool(const Person&)> condition;
    std::function<void(Person&)> effect;
};

namespace {
std::mt19937& Rng() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

int RandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(Rng());
}

void PauseShort() {
    std::this_thread::sleep_for(std::chrono::milliseconds(160));
}

int SafeReadInt() {
    int value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "请输入有效的数字：";
    }
    return value;
}

void ClampCoreStats(Person& role) {
    const auto clamp = [](int v, int low, int high) {
        if (v < low) return low;
        if (v > high) return high;
        return v;
    };
    role.health = clamp(role.health, -50, 200);
    role.energy = clamp(role.energy, -60, 200);
    role.mood = clamp(role.mood, -50, 200);
    role.knowledge = clamp(role.knowledge, 0, 2000);
}

void ShowStatus(const Person& role) {
    std::cout << "-----------------------------\n";
    std::cout << "第 " << role.day << " 天 | 年龄: " << role.age << "\n";
    std::cout << "姓名: " << role.name << "\n";
    std::cout << "学识: " << role.knowledge << "  健康: " << role.health << "  精力: " << role.energy
              << "  心情: " << role.mood << "  金钱: " << role.money << "\n";
    std::cout << "-----------------------------\n";
}

void UnlockAchievement(std::vector<std::string>& achievements, const std::string& ach) {
    if (std::find(achievements.begin(), achievements.end(), ach) == achievements.end()) {
        achievements.push_back(ach);
        std::cout << "成就解锁：" << ach << "！\n";
    }
}

void EvaluateAchievements(const Person& role, std::vector<std::string>& achievements) {
    if (role.knowledge >= 200) {
        UnlockAchievement(achievements, "学术启航");
    }
    if (role.money >= 2000) {
        UnlockAchievement(achievements, "小有积蓄");
    }
    if (role.health >= 120) {
        UnlockAchievement(achievements, "体魄强健");
    }
    if (role.mood >= 120) {
        UnlockAchievement(achievements, "心态平衡");
    }
    if (role.knowledge >= 320 && role.health >= 90 && role.mood >= 90) {
        UnlockAchievement(achievements, "全面发展");
    }
}

void ShowAchievements(const std::vector<std::string>& achievements) {
    if (achievements.empty()) {
        std::cout << "你还没有任何成就，继续努力！\n";
        return;
    }
    std::cout << "已获得成就：\n";
    for (const auto& a : achievements) {
        std::cout << "- " << a << "\n";
    }
}

void UseItem(Person& role, std::vector<Item>& bag) {
    if (bag.empty()) {
        std::cout << "背包空空如也。\n";
        return;
    }
    std::cout << "可用道具：\n";
    for (size_t i = 0; i < bag.size(); ++i) {
        std::cout << i + 1 << ". " << bag[i].name << " —— " << bag[i].description << "\n";
    }
    std::cout << "输入编号使用，道具将被消耗（0 取消）：";
    const int choice = SafeReadInt();
    if (choice <= 0 || static_cast<size_t>(choice) > bag.size()) {
        std::cout << "取消使用。\n";
        return;
    }
    bag[choice - 1].effect(role);
    std::cout << "你使用了 " << bag[choice - 1].name << "。\n";
    bag.erase(bag.begin() + (choice - 1));
    ClampCoreStats(role);
}

void ShowShop(const std::vector<Item>& shopItems) {
    std::cout << "校园小卖部（输入编号购买，0 返回）：\n";
    for (size_t i = 0; i < shopItems.size(); ++i) {
        std::cout << i + 1 << ". " << shopItems[i].name << " —— " << shopItems[i].description << "，价格 "
                  << shopItems[i].cost << "\n";
    }
}

void EnterShop(Person& role, std::vector<Item>& bag, const std::vector<Item>& shopItems) {
    ShowStatus(role);
    ShowShop(shopItems);
    std::cout << "你的余额：" << role.money << "。请选择：";
    const int choice = SafeReadInt();
    if (choice <= 0 || static_cast<size_t>(choice) > shopItems.size()) {
        std::cout << "离开商店。\n";
        return;
    }
    const Item& selected = shopItems[choice - 1];
    if (role.money < selected.cost) {
        std::cout << "金钱不足，无法购买。\n";
        return;
    }
    role.money -= selected.cost;
    bag.push_back(selected);
    std::cout << "购买成功，" << selected.name << " 已加入背包。\n";
}

void CheckDailyDecay(Person& role) {
    if (role.energy < 20) {
        std::cout << "精力透支，健康 -2。\n";
        role.health -= 2;
    }
    if (role.mood < 15) {
        std::cout << "心情压抑，学习效率下降，学识 -3。\n";
        role.knowledge -= 3;
    }
    if (role.money < 0) {
        std::cout << "负债压力让你心情 -5。\n";
        role.mood -= 5;
    }
    ClampCoreStats(role);
}

void CheckEnding(Person& role, const std::vector<std::string>& achievements) {
    if (!role.keepPlaying) return;

    if (role.health <= 0) {
        std::cout << "健康耗尽，你倒下了。结局：病倒退学。\n";
        role.keepPlaying = false;
        return;
    }
    if (role.mood <= -10) {
        std::cout << "长期压抑导致情绪崩溃。结局：需要休学疗养。\n";
        role.keepPlaying = false;
        return;
    }
    if (role.energy <= -40) {
        std::cout << "过度劳累让你无力继续。结局：被迫退学。\n";
        role.keepPlaying = false;
        return;
    }
    if (role.age > 30) {
        std::cout << "超过最长学籍年限。结局：延毕被清退。\n";
        role.keepPlaying = false;
        return;
    }

    const bool hasBalance = role.knowledge >= 420 && role.health >= 40 && role.mood >= 30;
    const bool richPath = role.money >= 6000 && role.knowledge >= 260;
    const bool socialPath =
        std::find(achievements.begin(), achievements.end(), "全面发展") != achievements.end() &&
        std::find(achievements.begin(), achievements.end(), "心态平衡") != achievements.end();

    if (hasBalance) {
        std::cout << "你顺利完成毕业要求，拿到了学位证！结局：如期毕业。\n";
        role.keepPlaying = false;
        role.success = true;
        return;
    }
    if (richPath) {
        std::cout << "凭借兼职与投资，你提前实现财务自由，选择休学创业。结局：创业先锋。\n";
        role.keepPlaying = false;
        role.success = true;
        return;
    }
    if (socialPath && role.day > 120) {
        std::cout << "你在校园里人脉广泛，成为大家信赖的伙伴。结局：社交达人。\n";
        role.keepPlaying = false;
        role.success = true;
    }
}

void TriggerRandomEvent(Person& role, const std::vector<RandomEvent>& events) {
    std::vector<size_t> candidates;
    for (size_t i = 0; i < events.size(); ++i) {
        if (events[i].condition(role)) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) return;
    const size_t index = candidates[RandomInt(0, static_cast<int>(candidates.size() - 1))];
    std::cout << "随机事件：" << events[index].description << "\n";
    events[index].effect(role);
    ClampCoreStats(role);
}

void PlayTask(Person& role) {
    const int taskType = RandomInt(1, 4);
    std::cout << "今日额外任务：";
    if (taskType == 1) {
        std::cout << "论文冲刺\n";
        std::cout << "1. 全力攻克（学识+30 精力-18 心情-6）\n";
        std::cout << "2. 稳扎稳打（学识+18 精力-10 心情-2）\n";
        std::cout << "3. 另择他日（无变化）\n";
        const int c = SafeReadInt();
        if (c == 1) {
            role.knowledge += 30;
            role.energy -= 18;
            role.mood -= 6;
        } else if (c == 2) {
            role.knowledge += 18;
            role.energy -= 10;
            role.mood -= 2;
        } else {
            std::cout << "你选择保持节奏，任务留到以后。\n";
        }
    } else if (taskType == 2) {
        std::cout << "体育打卡\n";
        std::cout << "1. 高强度训练（健康+25 精力-12 心情+3）\n";
        std::cout << "2. 轻松慢跑（健康+12 精力-6 心情+8）\n";
        std::cout << "3. 缺席（心情-2）\n";
        const int c = SafeReadInt();
        if (c == 1) {
            role.health += 25;
            role.energy -= 12;
            role.mood += 3;
        } else if (c == 2) {
            role.health += 12;
            role.energy -= 6;
            role.mood += 8;
        } else {
            role.mood -= 2;
            std::cout << "教练有些失望。\n";
        }
    } else if (taskType == 3) {
        std::cout << "社团活动\n";
        std::cout << "1. 全程组织（心情+18 学识+8 精力-10）\n";
        std::cout << "2. 协助支持（心情+10 学识+4 精力-4）\n";
        std::cout << "3. 不参与（无变化）\n";
        const int c = SafeReadInt();
        if (c == 1) {
            role.mood += 18;
            role.knowledge += 8;
            role.energy -= 10;
        } else if (c == 2) {
            role.mood += 10;
            role.knowledge += 4;
            role.energy -= 4;
        } else {
            std::cout << "你决定把时间留给自己。\n";
        }
    } else {
        std::cout << "兼职班次\n";
        std::cout << "1. 连轴上班（钱+160 精力-16 心情-6 健康-2）\n";
        std::cout << "2. 晚班替班（钱+90 精力-9 心情-2）\n";
        std::cout << "3. 放弃班次（无变化）\n";
        const int c = SafeReadInt();
        if (c == 1) {
            role.money += 160;
            role.energy -= 16;
            role.mood -= 6;
            role.health -= 2;
        } else if (c == 2) {
            role.money += 90;
            role.energy -= 9;
            role.mood -= 2;
        } else {
            std::cout << "你婉拒了这次班次。\n";
        }
    }
    ClampCoreStats(role);
}

void RunTimeSlot(Person& role, std::vector<Item>& bag, const std::vector<Action>& actions,
                 const std::string& slotName) {
    std::cout << "\n=== " << slotName << " ===\n";
    ShowStatus(role);
    std::cout << "选择行动：\n";
    for (size_t i = 0; i < actions.size(); ++i) {
        std::cout << i + 1 << ". " << actions[i].name << " —— " << actions[i].description << "\n";
    }
    std::cout << actions.size() + 1 << ". 使用道具\n";
    std::cout << actions.size() + 2 << ". 进入小卖部\n";
    std::cout << "请输入选项：";
    int choice = SafeReadInt();
    if (choice >= 1 && static_cast<size_t>(choice) <= actions.size()) {
        actions[choice - 1].execute(role, bag);
    } else if (choice == static_cast<int>(actions.size()) + 1) {
        UseItem(role, bag);
    } else if (choice == static_cast<int>(actions.size()) + 2) {
        std::cout << "该选项将在每日结束时统一进入商店。\n";
    } else {
        std::cout << "无效选择，时间被浪费。\n";
        role.mood -= 2;
    }
    ClampCoreStats(role);
}

}  // namespace

int main() {
    Person role;
    role.age = RandomInt(20, 22);
    role.knowledge = RandomInt(60, 85);
    role.health = RandomInt(70, 100);
    role.energy = RandomInt(65, 95);
    role.mood = RandomInt(60, 100);
    role.money = RandomInt(300, 480);

    std::vector<Item> bag;
    std::vector<std::string> achievements;

    const std::vector<Item> shopItems{
        {"咖啡", "恢复15精力，但心情-2", 40, [](Person& p) { p.energy += 15, p.mood -= 2; }},
        {"维生素", "恢复12健康", 50, [](Person& p) { p.health += 12; }},
        {"速记手册", "学识+18，精力-5", 65, [](Person& p) { p.knowledge += 18, p.energy -= 5; }},
        {"音乐耳机", "心情+18", 80, [](Person& p) { p.mood += 18; }},
        {"能量棒礼包", "精力+25，健康-3", 95, [](Person& p) { p.energy += 25, p.health -= 3; }},
    };

    const std::vector<RandomEvent> randomEvents{
        {"实验灵感突现，导师表扬你。学识+25，心情+10。", [](const Person&) { return true; },
         [](Person& p) {
             p.knowledge += 25;
             p.mood += 10;
         }},
        {"夜跑摔了一跤。健康-12，心情-4。", [](const Person& p) { return p.health > 40; },
         [](Person& p) {
             p.health -= 12;
             p.mood -= 4;
         }},
        {"室友请你喝奶茶，心情+12，金钱-10。", [](const Person&) { return true; },
         [](Person& p) {
             p.mood += 12;
             p.money -= 10;
         }},
        {"临时助研机会，学识+15，金钱+80，精力-10。", [](const Person& p) { return p.energy > 15; },
         [](Person& p) {
             p.knowledge += 15;
             p.money += 80;
             p.energy -= 10;
         }},
        {"发烧在宿舍躺了一天。健康-18，精力-8。", [](const Person& p) { return p.health < 60; },
         [](Person& p) {
             p.health -= 18;
             p.energy -= 8;
         }},
        {"参加校级比赛获奖，学识+20，心情+16。", [](const Person& p) { return p.knowledge > 120; },
         [](Person& p) {
             p.knowledge += 20;
             p.mood += 16;
         }},
        {"帮同学解题收到红包，金钱+60，心情+6。", [](const Person& p) { return p.knowledge > 90; },
         [](Person& p) {
             p.money += 60;
             p.mood += 6;
         }},
        {"沉迷短视频耗费时间，心情+4，精力-8，学识-6。", [](const Person&) { return true; },
         [](Person& p) {
             p.mood += 4;
             p.energy -= 8;
             p.knowledge -= 6;
         }},
        {"主动组织团建，心情+14，获得人脉。", [](const Person& p) { return p.mood > 60; },
         [](Person& p) { p.mood += 14; }},
    };

    const std::vector<Action> actions{
        {"图书馆冲刺", "学识+15，精力-10，心情-3", [](Person& p, std::vector<Item>&) {
             p.knowledge += 15;
             p.energy -= 10;
             p.mood -= 3;
         }},
        {"午休+冥想", "精力+16，心情+8", [](Person& p, std::vector<Item>&) {
             p.energy += 16;
             p.mood += 8;
         }},
        {"校园跑步", "健康+12，精力-8，心情+4", [](Person& p, std::vector<Item>&) {
             p.health += 12;
             p.energy -= 8;
             p.mood += 4;
         }},
        {"社交放松", "心情+15，金钱-20，精力+4", [](Person& p, std::vector<Item>&) {
             p.mood += 15;
             p.money -= 20;
             p.energy += 4;
         }},
        {"兼职班", "金钱+90，精力-12，心情-4", [](Person& p, std::vector<Item>&) {
             p.money += 90;
             p.energy -= 12;
             p.mood -= 4;
         }},
        {"自习+项目", "学识+10，金钱+25，精力-9", [](Person& p, std::vector<Item>&) {
             p.knowledge += 10;
             p.money += 25;
             p.energy -= 9;
         }},
        {"健康餐备餐", "健康+8，金钱-10，心情+3", [](Person& p, std::vector<Item>&) {
             p.health += 8;
             p.money -= 10;
             p.mood += 3;
         }},
    };

    std::cout << "欢迎来到《青科浮生记·文字版》！\n请输入你的名字：";
    std::cin >> role.name;
    if (role.name == "牛马") {
        std::cout << "看清真相，起始资金 +800。\n";
        role.money += 800;
    }

    const std::vector<std::string> slots{"早晨", "中午", "下午", "傍晚", "深夜"};

    while (role.keepPlaying) {
        for (const auto& slot : slots) {
            RunTimeSlot(role, bag, actions, slot);
            EvaluateAchievements(role, achievements);
            TriggerRandomEvent(role, randomEvents);
            CheckEnding(role, achievements);
            if (!role.keepPlaying) break;
            PauseShort();
        }
        if (!role.keepPlaying) break;

        PlayTask(role);
        CheckDailyDecay(role);
        CheckEnding(role, achievements);
        if (!role.keepPlaying) break;

        std::cout << "是否在一天结束时进入小卖部？(1 是 / 0 否)：";
        const int enterShop = SafeReadInt();
        if (enterShop == 1) {
            EnterShop(role, bag, shopItems);
        }

        std::cout << "查看成就吗？(1 是 / 0 否)：";
        const int viewAch = SafeReadInt();
        if (viewAch == 1) {
            ShowAchievements(achievements);
        }

        // 每日收尾，基础消耗与成长
        role.day++;
        role.energy -= 6;
        role.mood -= 2;
        if (role.day % 30 == 0) {
            role.age++;
            std::cout << "时间流逝，你的年龄来到 " << role.age << " 岁。\n";
        }
        ClampCoreStats(role);
        CheckEnding(role, achievements);
    }

    std::cout << "游戏结束，感谢游玩！";
    if (role.success) {
        std::cout << "你获得了理想的结局。\n";
    } else {
        std::cout << "下次再试试新的策略吧。\n";
    }
    return 0;
}
