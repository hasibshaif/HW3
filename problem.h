#ifndef PROBLEM_H
#define PROBLEM_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Problem {
public:
    virtual ~Problem() = default;
    virtual std::string getQuestion() const = 0;
    virtual std::string getAnswer() const = 0;
};

class ArithmeticProblem : public Problem {
public:
    explicit ArithmeticProblem(std::string rawProblem);
    std::string getQuestion() const override;
    std::string getAnswer() const override;
    std::string getTopic() const;
    int getDifficulty() const;
    static std::vector<std::shared_ptr<Problem>> problemList(const std::string& filename);
private:
    std::string question;
    std::string answer;
    std::string topic;
    int difficulty;
};

class ProblemConstraint {
public:
    ProblemConstraint(std::function<int(const Problem&)> metric, int minValue, int maxValue);
    bool satisfied(const std::vector<std::shared_ptr<Problem>>& selection) const;
    bool wouldExceedMax(const std::vector<std::shared_ptr<Problem>>& selection, const Problem& newProblem) const;

private:
    std::function<int(const Problem&)> metric;
    int minValue;
    int maxValue;
};

class ProblemSelector {
public:
    virtual ~ProblemSelector() = default;
    virtual std::vector<std::shared_ptr<Problem>> select(
        const std::vector<std::shared_ptr<Problem>>& bank,
        int count,
        const std::vector<ProblemConstraint>& constraints) const = 0;
};

class RandomReshuffleSelector : public ProblemSelector {
public:
    std::vector<std::shared_ptr<Problem>> select(
        const std::vector<std::shared_ptr<Problem>>& bank,
        int count,
        const std::vector<ProblemConstraint>& constraints) const override;
};

class MathProblem : public Problem {
public:
    explicit MathProblem(std::string rawProblem);
    std::string getQuestion() const override;
    std::string getAnswer() const override;
    std::string getTopic() const;
    std::string getAuthor() const;
    bool isLong() const;
    static std::vector<std::shared_ptr<Problem>> problemList(const std::string& filename);
private:
    std::string question;
    std::string answer;
    std::string topic;
    std::string author;
    bool longProblem;
};

class SmartSelector : public ProblemSelector {
public:
    std::vector<std::shared_ptr<Problem>> select(
        const std::vector<std::shared_ptr<Problem>>& bank,
        int count,
        const std::vector<ProblemConstraint>& constraints) const override;
};

#endif