# chatgptcpp

C++20 minimal client library for OpenAI-compatible chat completions.

## Build

Requires libcurl.

```bash
cmake -S . -B build
cmake --build build
```

## Usage

```cpp
#include "chatgptcpp/client.h"

int main() {
  chatgptcpp::OpenAIClient client("YOUR_API_KEY");
  chatgptcpp::ChatCompletionRequest request;
  request.model = "gpt-4o-mini";
  request.messages = {
      {"system", "You are a helpful assistant."},
      {"user", "Hello!"}
  };
  request.temperature = 0.7;
  request.max_tokens = 128;

  const auto response = client.chat_completion_parsed(request);
  // response.choices[0].message.content ...
  // response.usage.total_tokens ...

  chatgptcpp::append_assistant_message(request, response);
  // request.messages now includes the assistant reply.
}
```

## Conversation example (rock-paper-scissors x3)

See `examples/rock_paper_scissors.cpp`.

## Notes

- `OpenAIClient` returns raw JSON response text.
- `base_url` defaults to `https://api.openai.com/v1` but can be changed for
  other OpenAI-compatible providers.
