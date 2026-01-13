#include "chatgptcpp/client.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

int main() {
  const char *api_key = std::getenv("OPENAI_API_KEY");
  if (!api_key || api_key[0] == '\0') {
    return 1;
  }
  chatgptcpp::OpenAIClient client(api_key);
  chatgptcpp::ChatCompletionRequest request;
  request.model = "gpt-4o-mini";
  request.messages = {
      {"system",
       "あなたは rock-paper-scissors の対戦相手です。"
       "ユーザーが 'Rock', 'Paper', 'Scissors' のいずれかを送信したら、"
       "それに対してランダムに 'Rock', 'Paper', 'Scissors' "
       "のいずれかを返答の最初のワードとして返してください。"}};

  const char *hands[] = {"Rock", "Paper", "Scissors"};

  const auto strip_newlines = [](std::string value) {
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
    return value;
  };

  for (int round = 0; round < 3; ++round) {
    const std::string user_hand = hands[round % 3];
    request.messages.push_back({"user", user_hand});
    const auto response = client.chat_completion_parsed(request);
    if (!chatgptcpp::append_assistant_message(request, response)) {
      break;
    }
    std::string ai_hand = strip_newlines(response.choices[0].message.content);
    ai_hand = ai_hand.find("Rock")       ? "Rock"
              : ai_hand.find("Paper")    ? "Paper"
              : ai_hand.find("Scissors") ? "Scissors"
                                         : "Unknown";

    std::string result = "draw";
    if ((user_hand == "Rock" && ai_hand == "Scissors") ||
        (user_hand == "Paper" && ai_hand == "Rock") ||
        (user_hand == "Scissors" && ai_hand == "Paper")) {
      result = "win";
    } else if (user_hand != ai_hand) {
      result = "lose";
    }
    std::cout << "Round " << (round + 1) << ": user=" << user_hand
              << " ai=" << ai_hand << " result=" << result << "\n";
  }
}
