#include <algorithm>
#include <chrono>
#include <cmath>
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
    int semesterDay = 1;
    int knowledge = 0;
    int health = 0;
    int energy = 0;
    int mood = 0;
    int money = 0;
    // 中文注释：日常增益和状态
    double studyFactor = 1.0;
    double healthFactor = 1.0;
    double moodFactor = 1.0;
    double moneyFactor = 1.0;
    int studyBoostDays = 0;     // 学习增益天数
    int moodGuardDays = 0;      // 心情保护天数
    int studyStreak = 0;        // 连续学习日
    bool studiedToday = false;  // 今日是否学习
    int energyDecay = 6;        // 难度影响的每日精力消耗
    int moodDecay = 2;          // 难度影响的每日心情消耗
    std::string planName = "自由日常";
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

struct DailyPlan {
    std::string name;
    double studyFactor = 1.0;
    double healthFactor = 1.0;
    double moodFactor = 1.0;
    double moneyFactor = 1.0;
    int energyOffset = 0;
};

namespace {
std::mt19937& Rng() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

int SafeReadInt();
void ClampCoreStats(Person& role);

int RandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(Rng());
}

void PauseShort() {
    std::this_thread::sleep_for(std::chrono::milliseconds(160));
}

// 中文注释：难度设定，影响每日衰减与初始资源
void ChooseDifficulty(Person& role) {
    std::cout << "请选择难度：\n1. 轻松（初始资源高，日常消耗低）\n2. 平衡（默认）\n3. 压力（资源少，消耗高，成就获取更快）\n";
    int choice = SafeReadInt();
    if (choice == 1) {
        role.energy += 10;
        role.health += 10;
        role.money += 200;
        role.energyDecay = 4;
        role.moodDecay = 1;
        role.planName = "轻松模式";
    } else if (choice == 3) {
        role.energy -= 10;
        role.health -= 8;
        role.money -= 100;
        role.energyDecay = 8;
        role.moodDecay = 3;
        role.planName = "压力模式";
    } else {
        role.planName = "平衡模式";
    }
    ClampCoreStats(role);
}

// 中文注释：每日计划选择，决定当天资源倾斜方向
void ChooseDailyPlan(Person& role) {
    std::cout << "\n为今天定个计划：\n";
    std::cout << "1. 学霸模式（学识收益+25%，心情正向-5%，额外消耗精力2）\n";
    std::cout << "2. 养生模式（健康收益+20%，心情收益+10%，娱乐花费-10%）\n";
    std::cout << "3. 财富模式（金币收益+20%，学识收益-10%，额外消耗精力1）\n";
    std::cout << "4. 心情模式（心情收益+25%，负面心情减半）\n";
    std::cout << "5. 自由发挥（无额外修正）\n";
    std::cout << "请选择：";
    int choice = SafeReadInt();
    role.studyFactor = 1.0;
    role.healthFactor = 1.0;
    role.moodFactor = 1.0;
    role.moneyFactor = 1.0;
    role.planName = "自由日常";
    int extraEnergy = 0;
    switch (choice) {
    case 1:
        role.studyFactor = 1.25;
        role.moodFactor = 0.95;
        role.planName = "学霸模式";
        extraEnergy = 2;
        break;
    case 2:
        role.healthFactor = 1.2;
        role.moodFactor = 1.1;
        role.moneyFactor = 0.9;
        role.planName = "养生模式";
        break;
    case 3:
        role.moneyFactor = 1.2;
        role.studyFactor = 0.9;
        role.planName = "财富模式";
        extraEnergy = 1;
        break;
    case 4:
        role.moodFactor = 1.25;
        role.moodGuardDays += 1;
        role.planName = "心情模式";
        break;
    default:
        break;
    }
    if (extraEnergy > 0) {
        role.energy -= extraEnergy;
    }
    role.studiedToday = false;
    ClampCoreStats(role);
}

// 中文注释：安全的数值加减，避免心情保护或溢出问题
void AddMood(Person& role, int delta) {
    int realDelta = delta;
    if (delta < 0 && role.moodGuardDays > 0) {
        // 心情保护降低负面影响
        realDelta = delta / 2;
        role.moodGuardDays--;
    }
    if (realDelta > 0) {
        realDelta = static_cast<int>(std::round(realDelta * role.moodFactor));
    }
    role.mood += realDelta;
}

void AddKnowledge(Person& role, int baseDelta, int extraEnergyCost = 0) {
    int delta = static_cast<int>(std::round(baseDelta * role.studyFactor));
    if (role.studyBoostDays > 0) {
        delta += 6;
        role.studyBoostDays--;
    }
    role.knowledge += delta;
    role.energy -= extraEnergyCost;
    role.studiedToday = true;
}

void AddHealth(Person& role, int delta) {
    int realDelta = delta;
    if (delta > 0) {
        realDelta = static_cast<int>(std::round(delta * role.healthFactor));
    }
    role.health += realDelta;
}

void AddMoney(Person& role, int delta) {
    int realDelta = delta;
    if (delta > 0) {
        realDelta = static_cast<int>(std::round(delta * role.moneyFactor));
    }
    role.money += realDelta;
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
    std::cout << "第 " << role.day << " 天 (学期内第 " << role.semesterDay << " 天) | 年龄: " << role.age << "\n";
    std::cout << "姓名: " << role.name << "\n";
    std::cout << "学识: " << role.knowledge << "  健康: " << role.health << "  精力: " << role.energy
              << "  心情: " << role.mood << "  金钱: " << role.money << "\n";
    std::cout << "当前计划: " << role.planName << " | 连续学习日: " << role.studyStreak << " 天\n";
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
        AddMood(role, -5);
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
            AddKnowledge(role, 30);
            role.energy -= 18;
            AddMood(role, -6);
        } else if (c == 2) {
            AddKnowledge(role, 18);
            role.energy -= 10;
            AddMood(role, -2);
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
            AddHealth(role, 25);
            role.energy -= 12;
            AddMood(role, 3);
        } else if (c == 2) {
            AddHealth(role, 12);
            role.energy -= 6;
            AddMood(role, 8);
        } else {
            AddMood(role, -2);
            std::cout << "教练有些失望。\n";
        }
    } else if (taskType == 3) {
        std::cout << "社团活动\n";
        std::cout << "1. 全程组织（心情+18 学识+8 精力-10）\n";
        std::cout << "2. 协助支持（心情+10 学识+4 精力-4）\n";
        std::cout << "3. 不参与（无变化）\n";
        const int c = SafeReadInt();
        if (c == 1) {
            AddMood(role, 18);
            AddKnowledge(role, 8);
            role.energy -= 10;
        } else if (c == 2) {
            AddMood(role, 10);
            AddKnowledge(role, 4);
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
            AddMoney(role, 160);
            role.energy -= 16;
            AddMood(role, -6);
            role.health -= 2;
        } else if (c == 2) {
            AddMoney(role, 90);
            role.energy -= 9;
            AddMood(role, -2);
        } else {
            std::cout << "你婉拒了这次班次。\n";
        }
    }
    ClampCoreStats(role);
}

// 中文注释：周末安排，提供额外成长与增益
void RunWeekend(Person& role, std::vector<Item>& bag, const std::vector<Item>& shopItems) {
    std::cout << "\n—— 周末来临，你可以自由安排 ——\n";
    std::cout << "1. 深度休息（健康+15 精力+22 心情+12）\n";
    std::cout << "2. 集中复习（学识+40 精力-14 心情-4，获得2天学习增益）\n";
    std::cout << "3. 兼职冲刺（金钱+260 精力-20 心情-6 健康-3）\n";
    std::cout << "4. 团建旅行（心情+26 健康+10 金钱-150，获得3天心情保护）\n";
    std::cout << "5. 周末逛店（直接进入小卖部）\n";
    std::cout << "请选择：";
    const int choice = SafeReadInt();
    switch (choice) {
    case 1:
        AddHealth(role, 15);
        role.energy += 22;
        AddMood(role, 12);
        break;
    case 2:
        AddKnowledge(role, 40);
        role.energy -= 14;
        AddMood(role, -4);
        role.studyBoostDays += 2;
        break;
    case 3:
        AddMoney(role, 260);
        role.energy -= 20;
        AddMood(role, -6);
        role.health -= 3;
        break;
    case 4:
        AddMood(role, 26);
        AddHealth(role, 10);
        AddMoney(role, -150);
        role.moodGuardDays += 3;
        break;
    case 5:
        EnterShop(role, bag, shopItems);
        break;
    default:
        std::cout << "你决定宅在宿舍，什么也没做。\n";
        AddMood(role, -1);
        break;
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
        AddMood(role, -2);
    }
    ClampCoreStats(role);
}

}  // namespace

// 中文注释：奖学金/补助检测
void CheckScholarship(Person& role, std::vector<std::string>& achievements) {
    if (role.knowledge >= 200 && role.mood >= 40 && role.day % 14 == 0) {
        const int grant = RandomInt(180, 380);
        std::cout << "你因学习表现获得奖学金 +" << grant << "！\n";
        role.money += grant;
        UnlockAchievement(achievements, "奖学金获得者");
    }
    if (role.health >= 110 && role.day % 10 == 0) {
        std::cout << "坚持锻炼让你状态良好，获得教练鼓励（心情+8）。\n";
        AddMood(role, 8);
    }
}

// 中文注释：月度考核，提供目标导向的奖励与惩罚
void CheckMonthlyAssessment(Person& role) {
    if (role.day % 14 != 0) return;
    const int target = 140 + (role.day / 14) * 20;
    std::cout << "月度考核：需要学识达到 " << target << "。\n";
    if (role.knowledge >= target) {
        std::cout << "考核通过，获得导师认可（心情+12，精力+6）。\n";
        AddMood(role, 12);
        role.energy += 6;
        role.studyBoostDays += 1;
    } else {
        std::cout << "考核未达标，导师提醒你抓紧复习（心情-10，精力-6）。\n";
        AddMood(role, -10);
        role.energy -= 6;
    }
    ClampCoreStats(role);
}

// 中文注释：每日结束的连击奖励
void HandleStudyStreak(Person& role) {
    if (role.studiedToday) {
        role.studyStreak++;
    } else {
        role.studyStreak = 0;
    }
    if (role.studyStreak > 0 && role.studyStreak % 3 == 0) {
        std::cout << "连续学习满 " << role.studyStreak << " 天，触发心流！学识获取额外提升，心情+6。\n";
        role.studyBoostDays += 2;
        AddMood(role, 6);
    }
    role.studiedToday = false;
    ClampCoreStats(role);
}

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
        {"咖啡", "恢复15精力，但心情-2", 40, [](Person& p) {
             p.energy += 15;
             AddMood(p, -2);
         }},
        {"维生素", "恢复12健康", 50, [](Person& p) { p.health += 12; }},
        {"速记手册", "学识+18，精力-5", 65, [](Person& p) {
             AddKnowledge(p, 18);
             p.energy -= 5;
         }},
        {"音乐耳机", "心情+18", 80, [](Person& p) { AddMood(p, 18); }},
        {"能量棒礼包", "精力+25，健康-3", 95, [](Person& p) { p.energy += 25, p.health -= 3; }},
    };

    const std::vector<RandomEvent> randomEvents{
        {"早上起晚撞上熬了大夜的僵尸老滕，给了他两拳，一想到他成天虐待小哲又补了两脚嘿嘿嘿。心情+10。",
         [](const Person&) { return true; }, [](Person& p) { AddMood(p, 10); }},
        {"你正在为写不出论文而发愁，莉莉说带你发顶刊，你激动地认她做干妈！学识+100，心情+10。",
         [](const Person&) { return true; }, [](Person& p) {
             AddKnowledge(p, 100);
             AddMood(p, 10);
         }},
        {"上班时间摸鱼打游戏被军哥发现，军哥说：“没事儿你先玩！”，你羞愧不已，默默关掉了手机。",
         [](const Person&) { return true; }, [](Person& p) { AddMood(p, -2); }},
        {"向英子请教学术问题，耗时一上午，她说：“你回去再研究研究，研究明白了给我讲讲；还有个表格辛苦你加加班”，心情-20。",
         [](const Person&) { return true; }, [](Person& p) { AddMood(p, -20); }},
        {"放假前夕，桃从办公室进进出出，焦急地问：“哎呀我靠，你们什么时候放假？”（无直接影响）",
         [](const Person&) { return true; }, [](Person&) {}},
        {"大力值班巡查时：“你们屋学习氛围真浓厚，咱们组真的是蒸蒸日上！”（无直接影响）",
         [](const Person&) { return true; }, [](Person&) {}},
        {"鉴定前期，小孙恬不知耻地克扣大家休息时间，还大言不惭地说道：“我们是一个team！”（心情-5）",
         [](const Person&) { return true; }, [](Person& p) { AddMood(p, -5); }},
        {"大老项陪完酒一瘸一拐地跑回来查人，发现一片混乱朝小孙怒吼：“XXX你能干干，不能干滚！”第二天全组喜提扣一个月工资。金钱-800！",
         [](const Person&) { return true; }, [](Person& p) { AddMoney(p, -800); }},
        {"实验灵感突现，导师表扬你。学识+25，心情+10。", [](const Person&) { return true; },
         [](Person& p) {
             AddKnowledge(p, 25);
             AddMood(p, 10);
         }},
        {"夜跑摔了一跤。健康-12，心情-4。", [](const Person& p) { return p.health > 40; },
         [](Person& p) {
             p.health -= 12;
             AddMood(p, -4);
         }},
        {"室友请你喝奶茶，心情+12，金钱-10。", [](const Person&) { return true; },
         [](Person& p) {
             AddMood(p, 12);
             AddMoney(p, -10);
         }},
        {"临时助研机会，学识+15，金钱+80，精力-10。", [](const Person& p) { return p.energy > 15; },
         [](Person& p) {
             AddKnowledge(p, 15);
             AddMoney(p, 80);
             p.energy -= 10;
         }},
        {"发烧在宿舍躺了一天。健康-18，精力-8。", [](const Person& p) { return p.health < 60; },
         [](Person& p) {
             p.health -= 18;
             p.energy -= 8;
         }},
        {"参加校级比赛获奖，学识+20，心情+16。", [](const Person& p) { return p.knowledge > 120; },
         [](Person& p) {
             AddKnowledge(p, 20);
             AddMood(p, 16);
         }},
        {"帮同学解题收到红包，金钱+60，心情+6。", [](const Person& p) { return p.knowledge > 90; },
         [](Person& p) {
             AddMoney(p, 60);
             AddMood(p, 6);
         }},
        {"沉迷短视频耗费时间，心情+4，精力-8，学识-6。", [](const Person&) { return true; },
         [](Person& p) {
             AddMood(p, 4);
             p.energy -= 8;
             p.knowledge -= 6;
         }},
        {"主动组织团建，心情+14，获得人脉。", [](const Person& p) { return p.mood > 60; },
         [](Person& p) { AddMood(p, 14); }},
        {"科研心流降临，未来三天学习效率提升！", [](const Person& p) { return p.knowledge > 80; },
         [](Person& p) { p.studyBoostDays += 3; }},
        {"心理辅导课让你学会调节，获得2天心情保护。", [](const Person& p) { return p.mood < 50; },
         [](Person& p) { p.moodGuardDays += 2; AddMood(p, 6); }},
        {"返校核酸排队一整天，精力-6，心情-4。", [](const Person&) { return true; },
         [](Person& p) {
             p.energy -= 6;
             AddMood(p, -4);
         }},
        {"系主任偶遇你，鼓励你申请项目，学识+12，心情+6。", [](const Person&) { return true; },
         [](Person& p) {
             AddKnowledge(p, 12);
             AddMood(p, 6);
         }},
    };

    const std::vector<Action> actions{
        {"图书馆冲刺", "学识+15，精力-10，心情-3", [](Person& p, std::vector<Item>&) {
             AddKnowledge(p, 15);
             p.energy -= 10;
             AddMood(p, -3);
         }},
        {"午休+冥想", "精力+16，心情+8", [](Person& p, std::vector<Item>&) {
             p.energy += 16;
             AddMood(p, 8);
         }},
        {"校园跑步", "健康+12，精力-8，心情+4", [](Person& p, std::vector<Item>&) {
             AddHealth(p, 12);
             p.energy -= 8;
             AddMood(p, 4);
         }},
        {"社交放松", "心情+15，金钱-20，精力+4", [](Person& p, std::vector<Item>&) {
             AddMood(p, 15);
             AddMoney(p, -20);
             p.energy += 4;
         }},
        {"兼职班", "金钱+90，精力-12，心情-4", [](Person& p, std::vector<Item>&) {
             AddMoney(p, 90);
             p.energy -= 12;
             AddMood(p, -4);
         }},
        {"自习+项目", "学识+10，金钱+25，精力-9", [](Person& p, std::vector<Item>&) {
             AddKnowledge(p, 10);
             AddMoney(p, 25);
             p.energy -= 9;
         }},
        {"健康餐备餐", "健康+8，金钱-10，心情+3", [](Person& p, std::vector<Item>&) {
             AddHealth(p, 8);
             AddMoney(p, -10);
             AddMood(p, 3);
         }},
        {"校园探索", "随机发现物资/灵感/好友", [](Person& p, std::vector<Item>&) {
             int r = RandomInt(1, 4);
             if (r == 1) {
                 AddMoney(p, 40);
                 std::cout << "你在草坪边捡到校友送的咖啡券，金钱+40。\n";
             } else if (r == 2) {
                 AddHealth(p, 6);
                 AddMood(p, 4);
                 std::cout << "你在操场吹风，身心放松，健康+6，心情+4。\n";
             } else if (r == 3) {
                 AddKnowledge(p, 8);
                 std::cout << "偶遇学长分享研究心得，学识+8。\n";
             } else {
                 AddMood(p, -3);
                 p.energy -= 4;
                 std::cout << "探索太久有点累，精力-4，心情-3。\n";
             }
         }},
    };

    std::cout << "欢迎来到《青科浮生记·文字版》！\n请输入你的名字：";
    std::cin >> role.name;
    if (role.name == "牛马") {
        std::cout << "看清真相，起始资金 +800。\n";
        role.money += 800;
    }
    ChooseDifficulty(role);

    const std::vector<std::string> slots{"早晨", "中午", "下午", "傍晚", "深夜"};

    while (role.keepPlaying) {
        ChooseDailyPlan(role);
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

        if (role.semesterDay % 7 == 0) {
            RunWeekend(role, bag, shopItems);
            CheckEnding(role, achievements);
            if (!role.keepPlaying) break;
        }

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
        role.semesterDay++;
        role.energy -= role.energyDecay;
        AddMood(role, -role.moodDecay);
        if (role.day % 30 == 0) {
            role.age++;
            std::cout << "时间流逝，你的年龄来到 " << role.age << " 岁。\n";
        }
        if (role.semesterDay > 120) {
            role.semesterDay = 1;
            std::cout << "新学期开始，重置学期计数。\n";
        }
        CheckScholarship(role, achievements);
        CheckMonthlyAssessment(role);
        HandleStudyStreak(role);
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
