#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "problem.h"

// ****************************************************************************
// Configuration details

// Variable information to be printed on the test
std::string CLASS = "Arithmetic";
std::string TERM = "Fall 2025";
std::string EXAM = "925";
std::string TIME = "Day";
std::string TITLE = "Final Exam";
std::string FORM = "A";

// Source file for problem bank
std::string BANK = "math_problems.tex";

// Filename for the created test
std::string FILENAME = "fancy_math_test.tex";

// Constraints on the problem choice.
int NUM_PROBLEMS = 10; // The test must have exactly 10 problems.

// tex files to include in the test file
std::string TEX_HEADER = "fancy_tex_header.tex";
std::string CONTENT_HEADER = "fancy_content_header.tex";

// ****************************************************************************

class HeaderWriter {
public:
    virtual ~HeaderWriter() = default;
    virtual void write(std::ostream& output, int problemCount) const = 0;
};

class LayoutStrategy {
public:
    virtual ~LayoutStrategy() = default;
    virtual void writeProblem(std::ostream& output, const Problem& problem, int index, int shortProblemIndex) const = 0;
    virtual void finish(std::ostream& output) const = 0;
};

class FancyHeaderWriter : public HeaderWriter {
public:
    FancyHeaderWriter(
        std::string texHeader,
        std::string contentHeader,
        std::string className,
        std::string term,
        std::string exam,
        std::string time,
        std::string title,
        std::string form)
        : texHeader(std::move(texHeader)),
          contentHeader(std::move(contentHeader)),
          className(std::move(className)),
          term(std::move(term)),
          exam(std::move(exam)),
          time(std::move(time)),
          title(std::move(title)),
          form(std::move(form)) {
    }

    void write(std::ostream& output, int problemCount) const override {
        output << "\\input{" << texHeader << "}\n";
        output << "\\newcommand{\\class}{" << className << "}\n";
        output << "\\newcommand{\\term}{" << term << "}\n";
        output << "\\newcommand{\\examno}{" << exam << "}\n";
        output << "\\newcommand{\\dayeve}{" << time << "}\n";
        output << "\\newcommand{\\formletter}{" << form << "}\n";
        output << "\\newcommand{\\numproblems}{" << problemCount << " }\n";
        output << "\\newcommand{\\testtitle}{" << title << "}\n";
        output << "\\input{" << contentHeader << "}\n";
    }

private:
    std::string texHeader;
    std::string contentHeader;
    std::string className;
    std::string term;
    std::string exam;
    std::string time;
    std::string title;
    std::string form;
};

class MathLayout : public LayoutStrategy {
public:
    void writeProblem(std::ostream& output, const Problem& problem, int index, int shortProblemIndex) const override {
        const auto* mathProblem = dynamic_cast<const MathProblem*>(&problem);
        if (!mathProblem) {
            return;
        }
        
        bool isLong = mathProblem->isLong();
        
        if (isLong) {
            output << "\\pagebreak\n\n";
        } else {
            // For short problems: first of each pair gets pagebreak (except index 0), second gets vspace
            if (shortProblemIndex == 0) {
                // First short problem - no pagebreak needed at document start
            } else if (shortProblemIndex % 2 == 0) {
                output << "\\pagebreak\n\n";
            } else {
                output << "\\vspace{350pt}\n\n";
            }
        }
        
        output << "\\item\\begin{tabular}[t]{p{5in} p{.3in} p{.8in}}\n";
        output << problem.getQuestion();
        output << "& & \\arabic{enumi}.\\hrulefill\n\\end{tabular}\n";
    }

    void finish(std::ostream& output) const override {
        output << "\\end{enumerate}\n\\end{document}";
    }
};

class TestGeneratorApp {
public:
    TestGeneratorApp(const ProblemSelector& selector, const HeaderWriter& header, const LayoutStrategy& layout)
        : selector(selector), header(header), layout(layout) {
    }

    bool generate(
        const std::vector<std::shared_ptr<Problem>>& bank,
        const std::vector<ProblemConstraint>& constraints,
        int count,
        const std::string& filename,
        const std::set<std::string>& authors,
        const std::set<std::string>& topics) const {
        std::vector<std::shared_ptr<Problem>> test = selector.select(bank, count, constraints);
        
        // Verify exactly the right number of problems were selected
        if (static_cast<int>(test.size()) != count) {
            std::cerr << "Selector returned " << test.size() << " problems, expected " << count << std::endl;
            return false;
        }
        
        // Validate that every author/topic in the bank appears exactly 1-2 times
        std::map<std::string, int> authorCounts;
        std::map<std::string, int> topicCounts;
        for (const auto& problem : test) {
            const auto* math = dynamic_cast<const MathProblem*>(problem.get());
            if (math) {
                authorCounts[math->getAuthor()]++;
                topicCounts[math->getTopic()]++;
            }
        }
        
        // Check all authors in the bank (missing ones will have count 0)
        for (const auto& author : authors) {
            int c = authorCounts[author];
            if (c != 1 && c != 2) {
                std::cerr << "Invalid author count: " << author << " has " << c << " problems (expected 1 or 2)" << std::endl;
                return false;
            }
        }
        
        // Check all topics in the bank (missing ones will have count 0)
        for (const auto& topic : topics) {
            int c = topicCounts[topic];
            if (c != 1 && c != 2) {
                std::cerr << "Invalid topic count: " << topic << " has " << c << " problems (expected 1 or 2)" << std::endl;
                return false;
            }
        }
        
        // Sort problems: short ones first, long ones last
        std::sort(test.begin(), test.end(), [](const std::shared_ptr<Problem>& a, const std::shared_ptr<Problem>& b) {
            const auto* mathA = dynamic_cast<const MathProblem*>(a.get());
            const auto* mathB = dynamic_cast<const MathProblem*>(b.get());
            if (!mathA || !mathB) {
                return false;
            }
            if (mathA->isLong() == mathB->isLong()) {
                return false;
            }
            return !mathA->isLong();
        });
        
        std::ofstream output(filename);
        if (!output.is_open()) {
            std::cerr << "Unable to open file." << std::endl;
            return false;
        }

        header.write(output, static_cast<int>(test.size()));
        
        int shortProblemIndex = 0;
        for (int i = 0; i < static_cast<int>(test.size()); ++i) {
            const auto* math = dynamic_cast<const MathProblem*>(test[i].get());
            int shortIdx = -1;
            if (math && !math->isLong()) {
                shortIdx = shortProblemIndex;
                shortProblemIndex++;
            }
            layout.writeProblem(output, *test[i], i, shortIdx);
        }
        
        layout.finish(output);
        return true;
    }

private:
    const ProblemSelector& selector;
    const HeaderWriter& header;
    const LayoutStrategy& layout;
};

int main() {
    std::vector<std::shared_ptr<Problem>> bank = MathProblem::problemList(BANK);
    
    std::set<std::string> topics;
    std::set<std::string> authors;
    for (const auto& problem : bank) {
        const auto* math = dynamic_cast<const MathProblem*>(problem.get());
        if (math) {
            topics.insert(math->getTopic());
            authors.insert(math->getAuthor());
        }
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> longDist(3, 4);
    
    int numLong = longDist(gen);
    
    // Calculate how many authors should appear twice to total exactly 10 problems
    int numAuthors = static_cast<int>(authors.size());
    int authorsWithTwo = NUM_PROBLEMS - numAuthors;
    
    // Check feasibility: need 5 <= numAuthors <= 10 for authors to be possible
    if (authorsWithTwo < 0 || authorsWithTwo > numAuthors) {
        std::cerr << "Impossible constraint: " << numAuthors << " authors cannot satisfy exactly " << NUM_PROBLEMS << " problems" << std::endl;
        return 1;
    }
    
    // Start all authors at 1, then randomly choose which ones get bumped to 2
    std::map<std::string, int> authorTargets;
    for (const std::string& author : authors) {
        authorTargets[author] = 1;
    }
    
    // Randomly select which authors get the extra problem (target = 2)
    std::vector<std::string> authorList(authors.begin(), authors.end());
    std::shuffle(authorList.begin(), authorList.end(), gen);
    for (int i = 0; i < authorsWithTwo; ++i) {
        authorTargets[authorList[i]] = 2;
    }
    
    // Calculate how many topics should appear twice to total exactly 10 problems
    int numTopics = static_cast<int>(topics.size());
    int topicsWithTwo = NUM_PROBLEMS - numTopics;
    
    // Check feasibility: need 5 <= numTopics <= 10 for topics to be possible
    if (topicsWithTwo < 0 || topicsWithTwo > numTopics) {
        std::cerr << "Impossible constraint: " << numTopics << " topics cannot satisfy exactly " << NUM_PROBLEMS << " problems" << std::endl;
        return 1;
    }
    
    // Start all topics at 1, then randomly choose which ones get bumped to 2
    std::map<std::string, int> topicTargets;
    for (const std::string& topic : topics) {
        topicTargets[topic] = 1;
    }
    
    // Randomly select which topics get the extra problem (target = 2)
    std::vector<std::string> topicList(topics.begin(), topics.end());
    std::shuffle(topicList.begin(), topicList.end(), gen);
    for (int i = 0; i < topicsWithTwo; ++i) {
        topicTargets[topicList[i]] = 2;
    }
    
    std::vector<ProblemConstraint> constraints;
    
    // Constraint for number of long problems
    constraints.emplace_back(
        [](const Problem& problem) {
            const auto* math = dynamic_cast<const MathProblem*>(&problem);
            if (!math) {
                return 0;
            }
            return math->isLong() ? 1 : 0;
        },
        numLong,
        numLong);
    
    // Constraints for each author: use the randomly assigned target (1 or 2)
    for (const auto& pair : authorTargets) {
        const std::string& author = pair.first;
        int target = pair.second;
        constraints.emplace_back(
            [author](const Problem& problem) {
                const auto* math = dynamic_cast<const MathProblem*>(&problem);
                if (!math) {
                    return 0;
                }
                return math->getAuthor() == author ? 1 : 0;
            },
            target,
            target);
    }
    
    // Constraints for each topic: use the randomly assigned target (1 or 2)
    for (const auto& pair : topicTargets) {
        const std::string& topic = pair.first;
        int target = pair.second;
        constraints.emplace_back(
            [topic](const Problem& problem) {
                const auto* math = dynamic_cast<const MathProblem*>(&problem);
                if (!math) {
                    return 0;
                }
                return math->getTopic() == topic ? 1 : 0;
            },
            target,
            target);
    }

    SmartSelector selector;
    FancyHeaderWriter header(TEX_HEADER, CONTENT_HEADER, CLASS, TERM, EXAM, TIME, TITLE, FORM);
    MathLayout layout;
    TestGeneratorApp generator(selector, header, layout);
    if (!generator.generate(bank, constraints, NUM_PROBLEMS, FILENAME, authors, topics)) {
        return 1;
    }
    
    return 0;
}

