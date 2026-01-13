#include "chatgptcpp/client.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>
#include <utility>

namespace chatgptcpp {
namespace {

class CurlGlobal {
 public:
  CurlGlobal() {
    const auto code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (code != CURLE_OK) {
      throw std::runtime_error("curl_global_init failed");
    }
  }

  ~CurlGlobal() { curl_global_cleanup(); }
};

size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

std::string ensure_trailing_slash(std::string base_url) {
  if (!base_url.empty() && base_url.back() != '/') {
    base_url.push_back('/');
  }
  return base_url;
}

}  // namespace

OpenAIClient::OpenAIClient(std::string api_key, std::string base_url)
    : api_key_(std::move(api_key)), base_url_(ensure_trailing_slash(std::move(base_url))) {}

void OpenAIClient::set_timeout_seconds(long seconds) {
  if (seconds <= 0) {
    throw std::invalid_argument("timeout must be positive");
  }
  timeout_seconds_ = seconds;
}

std::string OpenAIClient::chat_completion(const ChatCompletionRequest& request) const {
  static CurlGlobal curl_global_guard;

  CURL* curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("curl_easy_init failed");
  }

  std::string response_body;
  char error_buffer[CURL_ERROR_SIZE] = {};

  const std::string url = base_url_ + "chat/completions";
  const std::string payload = build_chat_request_json(request);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                   static_cast<curl_off_t>(payload.size()));
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds_);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

  struct curl_slist* headers = nullptr;
  std::string auth_header = "Authorization: Bearer " + api_key_;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, auth_header.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  const auto perform_code = curl_easy_perform(curl);

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (perform_code != CURLE_OK) {
    const char* message = error_buffer[0] != '\0' ? error_buffer
                                                  : curl_easy_strerror(perform_code);
    throw std::runtime_error(std::string("curl request failed: ") + message);
  }

  if (http_code >= 400) {
    std::ostringstream oss;
    oss << "http error " << http_code << ": " << response_body;
    throw std::runtime_error(oss.str());
  }

  return response_body;
}

ChatCompletionResponse OpenAIClient::chat_completion_parsed(
    const ChatCompletionRequest& request) const {
  return parse_chat_completion_response(chat_completion(request));
}

std::string OpenAIClient::build_chat_request_json(const ChatCompletionRequest& request) const {
  if (request.model.empty()) {
    throw std::invalid_argument("model is required");
  }
  if (request.messages.empty()) {
    throw std::invalid_argument("messages are required");
  }

  std::ostringstream oss;
  oss << "{";
  oss << "\"model\":\"" << json_escape(request.model) << "\",";
  oss << "\"messages\":[";
  for (size_t i = 0; i < request.messages.size(); ++i) {
    const auto& message = request.messages[i];
    if (message.role.empty()) {
      throw std::invalid_argument("message role is required");
    }
    oss << "{\"role\":\"" << json_escape(message.role) << "\","
        << "\"content\":\"" << json_escape(message.content) << "\"}";
    if (i + 1 < request.messages.size()) {
      oss << ",";
    }
  }
  oss << "]";
  if (request.temperature >= 0.0) {
    oss << ",\"temperature\":" << request.temperature;
  }
  if (request.max_tokens > 0) {
    oss << ",\"max_tokens\":" << request.max_tokens;
  }
  oss << "}";
  return oss.str();
}

ChatCompletionResponse OpenAIClient::parse_chat_completion_response(std::string_view body) {
  ChatCompletionResponse response;
  auto json = nlohmann::json::parse(body);

  response.id = json.value("id", "");
  response.model = json.value("model", "");
  response.created = json.value("created", 0L);

  if (json.contains("choices") && json["choices"].is_array()) {
    for (const auto& choice : json["choices"]) {
      ChatCompletionChoice parsed_choice;
      parsed_choice.index = choice.value("index", 0);
      parsed_choice.finish_reason = choice.value("finish_reason", "");

      if (choice.contains("message") && choice["message"].is_object()) {
        const auto& message = choice["message"];
        parsed_choice.message.role = message.value("role", "");
        parsed_choice.message.content = message.value("content", "");
      }
      response.choices.push_back(std::move(parsed_choice));
    }
  }

  if (json.contains("usage") && json["usage"].is_object()) {
    const auto& usage = json["usage"];
    response.usage.prompt_tokens = usage.value("prompt_tokens", 0);
    response.usage.completion_tokens = usage.value("completion_tokens", 0);
    response.usage.total_tokens = usage.value("total_tokens", 0);
  }

  return response;
}

bool append_assistant_message(ChatCompletionRequest& request,
                              const ChatCompletionResponse& response,
                              size_t choice_index) {
  if (choice_index >= response.choices.size()) {
    return false;
  }
  const auto& message = response.choices[choice_index].message;
  if (message.role.empty() || message.content.empty()) {
    return false;
  }
  if (message.role != "assistant") {
    return false;
  }
  request.messages.push_back(message);
  return true;
}

std::string OpenAIClient::json_escape(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for (const char ch : value) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (uch < 0x20) {
          const char* hex = "0123456789abcdef";
          out += "\\u00";
          out += hex[(uch >> 4) & 0x0f];
          out += hex[uch & 0x0f];
        } else {
          out += ch;
        }
        break;
    }
  }
  return out;
}

}  // namespace chatgptcpp
