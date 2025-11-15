#include <fstream>
#include <iostream>
#include <memory>
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
std::string BANK = "arithmetic_problems.tex";

// Filename for the created test
std::string FILENAME = "fancy_test.tex";

// Constraints on the problem choice.
int NUM_PROBLEMS = 20; // The test must have 20 problems.
int MIN_TOPIC = 3; // Each topic must be covered 
int MAX_TOPIC = 7; // by 3-7 problems.
int MIN_DIFFICULTY = 65; // Total difficulty (using the difficulty defined 
int MAX_DIFFICULTY = 75; // in the problem bank) must be 65-75.

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
    virtual void writeProblem(std::ostream& output, const Problem& problem, int index) const = 0;
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

class FancyLayout : public LayoutStrategy {
public:
    void writeProblem(std::ostream& output, const Problem& problem, int index) const override {
        int number = index + 1;
        if (number % 2 == 1) {
            output << "\\pagebreak\n\n";
        } else {
            output << "\\vspace{350pt}\n\n";
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
        const std::string& filename) const {
        std::vector<std::shared_ptr<Problem>> test = selector.select(bank, count, constraints);
        
        std::ofstream output(filename);
        if (!output.is_open()) {
            std::cerr << "Unable to open file." << std::endl;
            return false;
        }

        header.write(output, static_cast<int>(test.size()));
        
        for (int i = 0; i < static_cast<int>(test.size()); ++i) {
            layout.writeProblem(output, *test[i], i);
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
    std::vector<std::shared_ptr<Problem>> bank = ArithmeticProblem::problemList(BANK);
    std::set<std::string> topics;
    for (const auto& problem : bank) {
        const auto* arithmetic = dynamic_cast<const ArithmeticProblem*>(problem.get());
        if (arithmetic) {
            topics.insert(arithmetic->getTopic());
        }
    }

    std::vector<ProblemConstraint> constraints;
    constraints.emplace_back(
        [](const Problem& problem) {
            const auto* arithmetic = dynamic_cast<const ArithmeticProblem*>(&problem);
            if (!arithmetic) {
                return 0;
            }
            return arithmetic->getDifficulty();
        },
        MIN_DIFFICULTY,
        MAX_DIFFICULTY);
    for (const std::string& topic : topics) {
        constraints.emplace_back(
            [topic](const Problem& problem) {
                const auto* arithmetic = dynamic_cast<const ArithmeticProblem*>(&problem);
                if (!arithmetic) {
                    return 0;
                }
                return arithmetic->getTopic() == topic ? 1 : 0;
            },
            MIN_TOPIC,
            MAX_TOPIC);
    }

    RandomReshuffleSelector selector;
    FancyHeaderWriter header(TEX_HEADER, CONTENT_HEADER, CLASS, TERM, EXAM, TIME, TITLE, FORM);
    FancyLayout layout;
    TestGeneratorApp generator(selector, header, layout);
    if (!generator.generate(bank, constraints, NUM_PROBLEMS, FILENAME)) {
        return 1;
    }
    
    return 0;
}
