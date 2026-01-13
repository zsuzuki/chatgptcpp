#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace chatgptcpp {

struct Message {
  std::string role;
  std::string content;
};

struct ResponseMessage {
  std::string role;
  std::string content;
  std::string think;
  std::string raw_content;
};

struct ChatCompletionRequest {
  std::string model;
  std::vector<Message> messages;
  double temperature = 1.0;
  int max_tokens = 0;
};

struct ChatCompletionChoice {
  int index = 0;
  ResponseMessage message;
  std::string finish_reason;
};

struct ChatCompletionUsage {
  int prompt_tokens = 0;
  int completion_tokens = 0;
  int total_tokens = 0;
};

struct ChatCompletionResponse {
  std::string id;
  std::string model;
  long created = 0;
  std::vector<ChatCompletionChoice> choices;
  ChatCompletionUsage usage;
};

bool append_assistant_message(ChatCompletionRequest &request,
                              const ChatCompletionResponse &response,
                              size_t choice_index = 0);

class OpenAIClient {
public:
  explicit OpenAIClient(std::string api_key,
                        std::string base_url = "https://api.openai.com/v1");

  void set_timeout_seconds(long seconds);

  std::string chat_completion(const ChatCompletionRequest &request) const;
  ChatCompletionResponse
  chat_completion_parsed(const ChatCompletionRequest &request) const;
  static ChatCompletionResponse
  parse_chat_completion_response(std::string_view body);

private:
  std::string api_key_;
  std::string base_url_;
  long timeout_seconds_ = 60;

  std::string
  build_chat_request_json(const ChatCompletionRequest &request) const;
  static std::string json_escape(std::string_view value);
};

} // namespace chatgptcpp
