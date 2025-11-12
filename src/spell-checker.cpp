#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

/* TIMING UTILITY */
class TicToc {
  std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;

 public:
  TicToc() : m_begin(std::chrono::high_resolution_clock::now()) {}

  ~TicToc() {
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - m_begin)
            .count();
    std::clog << duration / 1000.0 << "ms\n";
  }
};

/* DATA STRUCTURES */
struct WordInfo {
  size_t start;
  size_t length;
  bool isCorrect;

  WordInfo(size_t s, size_t l, bool correct)
      : start(s), length(l), isCorrect(correct) {}
};

/* STRING UTILITIES */
inline bool isWordChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '\'' || c == '-';
}

bool equalsIgnoreCase(std::string_view str,
                      const std::unordered_set<std::string>& dict) {
  std::string lower;
  lower.reserve(str.length());
  for (char c : str) {
    lower += std::tolower(static_cast<unsigned char>(c));
  }
  return dict.find(lower) != dict.end();
}

inline bool hasLetters(std::string_view str) {
  for (char c : str) {
    if (std::isalpha(static_cast<unsigned char>(c))) return true;
  }
  return false;
}

inline bool hasDigits(std::string_view str) {
  for (char c : str) {
    if (std::isdigit(static_cast<unsigned char>(c))) return true;
  }
  return false;
}

bool isNumber(std::string_view str) {
  if (str.empty()) return false;

  bool hasDigit = false;
  for (char c : str) {
    if (std::isdigit(static_cast<unsigned char>(c))) {
      hasDigit = true;
    } else if (c != ',' && c != '.' && c != '-' && c != '+') {
      return false;
    }
  }
  return hasDigit;
}

void htmlEscapeInline(std::string& output, std::string_view text) {
  for (char c : text) {
    switch (c) {
      case '&':
        output += "&amp;";
        break;
      case '<':
        output += "&lt;";
        break;
      case '>':
        output += "&gt;";
        break;
      default:
        output += c;
        break;
    }
  }
}

/* DICTIONARY OPERATIONS */
std::unordered_set<std::string> loadDictionary(const std::string& filename) {
  std::unordered_set<std::string> dictionary;
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Error: Cannot open dictionary file: " << filename
              << std::endl;
    return dictionary;
  }

  std::string word;
  dictionary.reserve(400000);

  while (file >> word) {
    std::transform(word.begin(), word.end(), word.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    dictionary.insert(std::move(word));
  }

  std::clog << "Dictionary loaded: " << dictionary.size() << " words"
            << std::endl;
  return dictionary;
}

bool isWordValid(std::string_view word,
                 const std::unordered_set<std::string>& dictionary) {
  if (word.empty()) return true;
  if (isNumber(word)) return true;

  // Check for decade notation like "1960s" or "mid-1970s"
  if (word.length() >= 2 && word.back() == 's' &&
      std::isdigit(static_cast<unsigned char>(word[word.length() - 2]))) {
    // Check if everything before 's' is a valid number or hyphenated word
    // ending in number
    std::string_view withoutS = word.substr(0, word.length() - 1);
    bool allValidChars = true;
    for (char c : withoutS) {
      if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-') {
        allValidChars = false;
        break;
      }
    }
    if (allValidChars && hasDigits(withoutS)) return true;
  }

  // Combinations like "p34r" are invalid, but allow "mid-1970s"
  if (hasLetters(word) && hasDigits(word)) {
    // Check if it's a hyphenated word with letters and numbers
    size_t hyphenPos = word.find('-');
    if (hyphenPos != std::string_view::npos) {
      // Split and check each part
      std::string_view part1 = word.substr(0, hyphenPos);
      std::string_view part2 = word.substr(hyphenPos + 1);

      // If first part is valid word and second is number+s, allow it
      if (equalsIgnoreCase(part1, dictionary) && hasDigits(part2)) {
        return true;
      }
    }
    return false;
  }

  if (equalsIgnoreCase(word, dictionary)) return true;

  // Handle hyphenated compound words: check if both parts are valid
  size_t hyphenPos = word.find('-');
  if (hyphenPos != std::string_view::npos && hyphenPos > 0 &&
      hyphenPos < word.length() - 1) {
    std::string_view part1 = word.substr(0, hyphenPos);
    std::string_view part2 = word.substr(hyphenPos + 1);
    if (equalsIgnoreCase(part1, dictionary) &&
        equalsIgnoreCase(part2, dictionary)) {
      return true;
    }
  }

  // Handle possessives
  if (word.length() > 2) {
    if (word.back() == 's' && word[word.length() - 2] == '\'') {
      if (equalsIgnoreCase(word.substr(0, word.length() - 2), dictionary))
        return true;
    }
    if (word.back() == '\'' && word[word.length() - 2] == 's') {
      if (equalsIgnoreCase(word.substr(0, word.length() - 1), dictionary))
        return true;
    }
  }

  // Handle contractions
  size_t apostrophePos = word.find('\'');
  if (apostrophePos != std::string_view::npos) {
    static const std::vector<std::string_view> endings = {
        "n't", "'t", "'re", "'ve", "'ll", "'d", "'m", "'s"};

    for (const auto& ending : endings) {
      if (word.length() > ending.length()) {
        std::string_view suffix = word.substr(word.length() - ending.length());

        bool matches = true;
        for (size_t i = 0; i < ending.length(); ++i) {
          if (std::tolower(static_cast<unsigned char>(suffix[i])) !=
              ending[i]) {
            matches = false;
            break;
          }
        }

        if (matches) {
          std::string_view base = word.substr(0, apostrophePos);
          if (equalsIgnoreCase(base, dictionary)) return true;

          // Common irregular contractions
          std::string baseLower;
          baseLower.reserve(base.length());
          for (char c : base) {
            baseLower += std::tolower(static_cast<unsigned char>(c));
          }

          if (baseLower == "won" || baseLower == "can" || baseLower == "don" ||
              baseLower == "doesn" || baseLower == "didn" ||
              baseLower == "shouldn" || baseLower == "wouldn" ||
              baseLower == "couldn" || baseLower == "isn" ||
              baseLower == "aren" || baseLower == "wasn" ||
              baseLower == "weren" || baseLower == "hasn" ||
              baseLower == "haven" || baseLower == "hadn") {
            return true;
          }
        }
      }
    }
  }

  return false;
}

/* TEXT PROCESSING */
void extractWords(std::string_view text, size_t baseOffset,
                  const std::unordered_set<std::string>& dictionary,
                  std::vector<WordInfo>& words) {
  size_t wordStart = 0;
  bool inWord = false;

  for (size_t i = 0; i < text.length(); ++i) {
    if (isWordChar(text[i])) {
      if (!inWord) {
        wordStart = i;
        inWord = true;
      }
    } else {
      if (inWord) {
        std::string_view word = text.substr(wordStart, i - wordStart);
        if (hasLetters(word) || hasDigits(word)) {
          bool valid = isWordValid(word, dictionary);
          words.emplace_back(baseOffset + wordStart, i - wordStart, valid);
        }
        inWord = false;
      }
    }
  }

  if (inWord) {
    std::string_view word = text.substr(wordStart, text.length() - wordStart);
    if (hasLetters(word) || hasDigits(word)) {
      bool valid = isWordValid(word, dictionary);
      words.emplace_back(baseOffset + wordStart, text.length() - wordStart,
                         valid);
    }
  }
}

/* HTML GEN */
void generateHTML(std::string_view inputText,
                  const std::vector<WordInfo>& words) {
  std::string output;
  output.reserve(inputText.length() * 1.3);

  output += "<html>\n";

  size_t lastPos = 0;

  for (const auto& wordInfo : words) {
    // Output text before this word
    if (wordInfo.start > lastPos) {
      std::string_view between =
          inputText.substr(lastPos, wordInfo.start - lastPos);

      // Convert newlines to <br> tags
      for (char c : between) {
        if (c == '\n') {
          output += "<br>\n";
        } else {
          switch (c) {
            case '&':
              output += "&amp;";
              break;
            case '<':
              output += "&lt;";
              break;
            case '>':
              output += "&gt;";
              break;
            default:
              output += c;
              break;
          }
        }
      }
    }

    std::string_view word = inputText.substr(wordInfo.start, wordInfo.length);
    if (!wordInfo.isCorrect) {
      output += "<a style=\"color:red\">";
      htmlEscapeInline(output, word);
      output += "</a>";
    } else {
      htmlEscapeInline(output, word);
    }

    lastPos = wordInfo.start + wordInfo.length;
  }

  // Remaining text
  if (lastPos < inputText.length()) {
    std::string_view remaining = inputText.substr(lastPos);
    for (char c : remaining) {
      if (c == '\n') {
        output += "<br>\n";
      } else {
        switch (c) {
          case '&':
            output += "&amp;";
            break;
          case '<':
            output += "&lt;";
            break;
          case '>':
            output += "&gt;";
            break;
          default:
            output += c;
            break;
        }
      }
    }
  }

  output += "</html>\n";
  std::cout << output;
}

/* MAIN */
int main(int argc, char* argv[]) {
  TicToc timer;

  // Set stdout to binary mode on Windows to prevent \n -> \r\n conversion
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <dictionary_file> < input.txt > output.html" << std::endl;
    return 1;
  }

  std::clog << "Loading dictionary..." << std::endl;
  auto dictionary = loadDictionary(argv[1]);

  if (dictionary.empty()) {
    std::cerr << "Error: Dictionary is empty or could not be loaded."
              << std::endl;
    return 1;
  }

  std::clog << "Reading input text..." << std::endl;
  std::string inputText;
  {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    inputText = buffer.str();
  }
  std::clog << "Input size: " << inputText.length() << " bytes" << std::endl;

  unsigned int numThreads = std::thread::hardware_concurrency();
  if (numThreads == 0) numThreads = 4;
  std::clog << "Using " << numThreads << " threads" << std::endl;

  std::vector<WordInfo> results;

  if (inputText.length() < 100000 || numThreads == 1) {
    std::clog << "Processing with single thread..." << std::endl;
    results.reserve(inputText.length() / 5);
    extractWords(inputText, 0, dictionary, results);
  } else {
    std::clog << "Processing with multiple threads..." << std::endl;
    size_t chunkSize = inputText.length() / numThreads;
    std::vector<std::thread> threads;
    std::vector<std::vector<WordInfo>> threadResults(numThreads);

    for (unsigned int i = 0; i < numThreads; ++i) {
      size_t start = i * chunkSize;
      size_t end =
          (i == numThreads - 1) ? inputText.length() : (i + 1) * chunkSize;

      // Adjust boundaries to avoid splitting words
      if (end < inputText.length()) {
        while (end < inputText.length() && isWordChar(inputText[end])) {
          end++;
        }
      }

      threadResults[i].reserve((end - start) / 5);

      threads.emplace_back([&, i, start, end]() {
        std::string_view chunk =
            std::string_view(inputText).substr(start, end - start);
        extractWords(chunk, start, dictionary, threadResults[i]);
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    // Merge results (already in order due to baseOffset)
    size_t totalSize = 0;
    for (const auto& tr : threadResults) {
      totalSize += tr.size();
    }
    results.reserve(totalSize);

    for (auto& tr : threadResults) {
      results.insert(results.end(), std::make_move_iterator(tr.begin()),
                     std::make_move_iterator(tr.end()));
    }
  }

  std::clog << "Generating HTML..." << std::endl;
  generateHTML(inputText, results);

  std::clog << "Done! ";
  return 0;
}