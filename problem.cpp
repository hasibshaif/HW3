#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "problem.h"

std::vector<std::string> split(const std::string& s, const std::string& del) {
    std::vector<std::string> v;
    size_t start = 0;
    size_t end = s.find(del, start);
    while (end != std::string::npos) {
        v.push_back(s.substr(start, end - start));
        start = end + del.size();
        end = s.find(del, start);
    }
    v.push_back(s.substr(start));
    return v;
}

std::string FORMAT = "^([\\s\\S]*)\\\\answer\\{([\\s\\S]*)\\}[\\s\\S]*\\\\topic\\{(.*)\\}[\\s\\S]*\\\\difficulty\\{(.*)\\}";
std::regex re(FORMAT);

ArithmeticProblem::ArithmeticProblem(std::string rawProblem) {
    std::smatch match;
    if (!std::regex_search(rawProblem, match, re) == true) {
        std::cerr << "Invalid problem: " << rawProblem;
        throw std::runtime_error("Invalid problem");
    }
    question = match.str(1);
    answer = match.str(2);
    topic = match.str(3);
    difficulty = std::stoi(match.str(4));
}

std::string ArithmeticProblem::getQuestion() const {
    return question;
}

std::string ArithmeticProblem::getAnswer() const {
    return answer;
}

std::string ArithmeticProblem::getTopic() const {
    return topic;
}

int ArithmeticProblem::getDifficulty() const {
    return difficulty;
}

std::vector<std::shared_ptr<Problem>> ArithmeticProblem::problemList(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening the problem list" << std::endl;
        throw std::runtime_error("Cannot open problem list");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string str = buffer.str();
    std::vector<std::string> problemStrings = split(str, "\\item");
    // remove first element
    if (!problemStrings.empty()) {
        problemStrings.erase(problemStrings.begin());
    }

    std::vector<std::shared_ptr<Problem>> problems;
    for (const std::string& problemString : problemStrings) {
        problems.push_back(std::make_shared<ArithmeticProblem>(problemString));
    }

    return problems;
}

ProblemConstraint::ProblemConstraint(std::function<int(const Problem&)> metric, int minValue, int maxValue)
    : metric(std::move(metric)), minValue(minValue), maxValue(maxValue) {
}

bool ProblemConstraint::satisfied(const std::vector<std::shared_ptr<Problem>>& selection) const {
    int total = 0;
    for (const auto& problem : selection) {
        total += metric(*problem);
    }
    if (total < minValue || total > maxValue) {
        return false;
    }
    return true;
}

bool ProblemConstraint::wouldExceedMax(const std::vector<std::shared_ptr<Problem>>& selection, const Problem& newProblem) const {
    int total = 0;
    for (const auto& problem : selection) {
        total += metric(*problem);
    }
    total += metric(newProblem);
    return total > maxValue;
}

std::vector<std::shared_ptr<Problem>> RandomReshuffleSelector::select(
    const std::vector<std::shared_ptr<Problem>>& bank,
    int count,
    const std::vector<ProblemConstraint>& constraints) const {
    
    if (count > static_cast<int>(bank.size())) {
        throw std::runtime_error("Requested more problems than available");
    }

    std::vector<std::shared_ptr<Problem>> copy = bank;
    std::random_device rd;
    std::mt19937 gen(rd());

    while (true) {
        std::shuffle(copy.begin(), copy.end(), gen);
        std::vector<std::shared_ptr<Problem>> candidate(copy.begin(), copy.begin() + count);
        bool ok = true;
        for (const auto& constraint : constraints) {
            if (!constraint.satisfied(candidate)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            return candidate;
        }
    }
}

std::string MATH_FORMAT = "^([\\s\\S]*)\\\\answer\\{([\\s\\S]*)\\}[\\s\\S]*\\\\topic\\{(.*)\\}[\\s\\S]*\\\\author\\{(.*)\\}[\\s\\S]*\\\\isLong\\{(.*)\\}";
std::regex math_re(MATH_FORMAT);

MathProblem::MathProblem(std::string rawProblem) {
    std::smatch match;
    if (!std::regex_search(rawProblem, match, math_re) == true) {
        std::cerr << "Invalid problem: " << rawProblem;
        throw std::runtime_error("Invalid problem");
    }
    question = match.str(1);
    answer = match.str(2);
    topic = match.str(3);
    author = match.str(4);
    std::string longStr = match.str(5);
    longProblem = (longStr == "true");
}

std::string MathProblem::getQuestion() const {
    return question;
}

std::string MathProblem::getAnswer() const {
    return answer;
}

std::string MathProblem::getTopic() const {
    return topic;
}

std::string MathProblem::getAuthor() const {
    return author;
}

bool MathProblem::isLong() const {
    return longProblem;
}

std::vector<std::shared_ptr<Problem>> MathProblem::problemList(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening the problem list" << std::endl;
        throw std::runtime_error("Cannot open problem list");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string str = buffer.str();
    std::vector<std::string> problemStrings = split(str, "\\item");
    // remove first element
    if (!problemStrings.empty()) {
        problemStrings.erase(problemStrings.begin());
    }

    std::vector<std::shared_ptr<Problem>> problems;
    for (const std::string& problemString : problemStrings) {
        problems.push_back(std::make_shared<MathProblem>(problemString));
    }

    return problems;
}

std::vector<std::shared_ptr<Problem>> SmartSelector::select(
    const std::vector<std::shared_ptr<Problem>>& bank,
    int count,
    const std::vector<ProblemConstraint>& constraints) const {
    
    if (count > static_cast<int>(bank.size())) {
        throw std::runtime_error("Requested more problems than available");
    }

    std::vector<std::shared_ptr<Problem>> copy = bank;
    std::random_device rd;
    std::mt19937 gen(rd());

    while (true) {
        std::shuffle(copy.begin(), copy.end(), gen);
        std::vector<std::shared_ptr<Problem>> candidate;
        
        for (const auto& problem : copy) {
            if (static_cast<int>(candidate.size()) >= count) {
                break;
            }
            
            bool wouldExceedMax = false;
            for (const auto& constraint : constraints) {
                if (constraint.wouldExceedMax(candidate, *problem)) {
                    wouldExceedMax = true;
                    break;
                }
            }
            
            if (!wouldExceedMax) {
                candidate.push_back(problem);
            }
        }
        
        if (static_cast<int>(candidate.size()) == count) {
            bool ok = true;
            for (const auto& constraint : constraints) {
                if (!constraint.satisfied(candidate)) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                return candidate;
            }
        }
    }
}