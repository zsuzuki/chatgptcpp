#include "chatgptcpp/client.h"

#include <cassert>
#include <string>

int main() {
  const std::string sample = R"({
    "id": "chatcmpl-123",
    "model": "gpt-4o-mini",
    "created": 1710000000,
    "choices": [
      {
        "index": 0,
        "message": { "role": "assistant", "content": "Hello there!" },
        "finish_reason": "stop"
      },
      {
        "index": 1,
        "message": { "role": "assistant", "content": "Second choice." },
        "finish_reason": "length"
      }
    ],
    "usage": {
      "prompt_tokens": 5,
      "completion_tokens": 3,
      "total_tokens": 8
    }
  })";

  const auto response = chatgptcpp::OpenAIClient::parse_chat_completion_response(sample);

  assert(response.id == "chatcmpl-123");
  assert(response.model == "gpt-4o-mini");
  assert(response.created == 1710000000);
  assert(response.choices.size() == 2);
  assert(response.choices[0].index == 0);
  assert(response.choices[0].message.role == "assistant");
  assert(response.choices[0].message.content == "Hello there!");
  assert(response.choices[0].finish_reason == "stop");
  assert(response.choices[1].index == 1);
  assert(response.choices[1].message.role == "assistant");
  assert(response.choices[1].message.content == "Second choice.");
  assert(response.choices[1].finish_reason == "length");
  assert(response.usage.prompt_tokens == 5);
  assert(response.usage.completion_tokens == 3);
  assert(response.usage.total_tokens == 8);

  chatgptcpp::ChatCompletionRequest request;
  request.model = "gpt-4o-mini";
  request.messages = {{"user", "Hello"}};

  const bool appended = chatgptcpp::append_assistant_message(request, response, 0);
  assert(appended);
  assert(request.messages.size() == 2);
  assert(request.messages[1].role == "assistant");
  assert(request.messages[1].content == "Hello there!");
  return 0;
}
