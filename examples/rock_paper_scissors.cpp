#include "chatgptcpp/client.h"

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
  request.messages = {{"system", "You are a rock-paper-scissors opponent."}};

  const char *hands[] = {"Rock", "Paper", "Scissors"};

  for (int round = 0; round < 3; ++round) {
    const std::string user_hand = hands[round % 3];
    request.messages.push_back({"user", user_hand});
    const auto response = client.chat_completion_parsed(request);
    if (!chatgptcpp::append_assistant_message(request, response)) {
      break;
    }
    const std::string ai_hand = response.choices[0].message.content;

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
