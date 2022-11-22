#include <iostream>
#include <typeinfo>
#include <vector>
#include <string>
#include <regex>
#include <stack>
#include <exception>
#include <fstream>

using namespace std;

extern "C" void win();

void win(){
  system("/bin/sh");
}

void (*win_ptr)() = &win;

class Tag {
public:
  Tag(const string& tagName)
    : tagName_(tagName)
  {}
  virtual ~Tag()
  {}

  const string& GetTagName() const
  {
    return tagName_;
  }

  virtual bool Render() = 0;

private:
  string tagName_;
};

class HtmlTag
  : public Tag
{
public:
  static constexpr const char* TagName = "html";

  HtmlTag()
    : Tag(TagName)
  {}

  bool Render() override
  {
    cout << "html tag rendered" << endl;
    return true;
  }
};

class MetaTag
  : public Tag
{
public:
  static constexpr const char* TagName = "meta";

  MetaTag()
    : Tag(TagName)
  {}

  bool Render() override
  {
    cout << "meta tag rendered" << endl;
    return true;
  }
private:
  void* dummy[1];
};

class PTag
  : public Tag
{
public:
  static constexpr const char* TagName = "p";

  PTag()
    : Tag(TagName)
  {}

  bool Render() override
  {
    cout << "p tag rendered" << endl;
    return true;
  }
  void* dummy;
};

//////////////////////////// DEBUG FUNCTIONS

template <typename T>
T* allocate()
{
  T* result = new T;
  const auto& t = typeid(T);
  cout << "[ALLOCATED] " << t.name() << "(" << hex << result << "-" << (void*)((char*)result + sizeof(T)) << "), with size " << sizeof(T) << endl;
  return result;
}

template <typename T>
void deallocate(T* obj)
{
  const auto& t = typeid(T);
  cout << "[FREED] " << t.name() << "(" << hex << obj << ")" << ", with size " << sizeof(T) << endl;
  delete obj;
}

//////////////////////////// DEBUG FUNCTIONS

class HtmlRender
{
public:
  HtmlRender(const string& htmlText)
    : root_(nullptr)
  {
    ParseHtml(htmlText);
  }

  void Rend()
  {
    Rend(root_);
  }

private:
  struct TreeNode
  {
    int64_t id;
    Tag* tag;
    vector<TreeNode*> childs;
    void* dummy;
  };

  void ParseHtml(const string& htmlText)
  {
    stack<TreeNode*> tagLevel;

    regex line_regex(R"END(<([/]?)(\S+)(\s+id\s*=\s*(\d+?))?\s*>[\r\n]*)END", regex_constants::ECMAScript | regex_constants::icase);
    auto lines_begin = sregex_iterator(htmlText.cbegin(), htmlText.cend(), line_regex);
    auto lines_end = sregex_iterator();
    for (auto i = lines_begin; i != lines_end; i++)
    {
      bool isOpening;
      string tagName, id;
      if (i->size() == 5)
      {
        // is opening tag or not
        if ((*i)[1].str().empty())
          isOpening = true;
        else
          isOpening = false;
        tagName = (*i)[2];
        id = (*i)[4];
      }
      else
      {
        continue;
      }

      bool rootTagExists = root_ != nullptr;
      bool isHtmlTag = tagName == HtmlTag::TagName;
      if (!rootTagExists && !isHtmlTag)
        throw runtime_error("First tag should be html");

      if (rootTagExists && tagLevel.empty())
        throw runtime_error("Only one root tag is allowed");

      if (isOpening)
      {
        TreeNode* node = allocate<TreeNode>();

        Tag* tag;
        if (tagName == HtmlTag::TagName)
          tag = allocate<HtmlTag>();
        else if (tagName == MetaTag::TagName)
          tag = allocate<MetaTag>();
        else if (tagName == PTag::TagName)
          tag = allocate<PTag>();
        else
          throw runtime_error("Only <html>, <meta> and <p> tags are supported");

        node->id = strtoull(id.data(), nullptr, 10);

        node->tag = tag;

        if (!rootTagExists)
        {
          cout << "Got root tag <" << node->tag->GetTagName() << ">" << endl;
          root_ = node;
        }
        else
        {
          cout << "Got tag <" << node->tag->GetTagName() << ">, child of <" << tagLevel.top()->tag->GetTagName() << ">" << endl;
          tagLevel.top()->childs.push_back(node);
        }

        tagLevel.push(node);
      }
      else
      {
        if (!rootTagExists)
          throw runtime_error("Closing tag doesn't have corresponding opening tag");

        if (tagName != tagLevel.top()->tag->GetTagName())
        {
          cout << "Invalid closing tag <" << tagName << "> of parent tag <" << tagLevel.top()->tag->GetTagName() << ">, deallocating" << endl;
          deallocate(tagLevel.top()->tag);
        }

        tagLevel.pop();
      }
    }

    if (root_ == nullptr)
      throw runtime_error("Html is empty");
  }

  void Rend(TreeNode* current)
  {
    cout << "Rendering tag..." << endl;
    current->tag->Render();
    for (auto it : current->childs)
      Rend(it);
  }

  TreeNode* root_;
};

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    cout << argv[0] << " - Parse and render HTML files" << endl
      << "How to use - just pass HTML file path as first argument!" << endl
      << "Example: ./uaf test.html" << endl;
    return 2;
  }

  try
  {
    ifstream f(argv[1]);
    string htmlText((istreambuf_iterator<char>(f)),
      istreambuf_iterator<char>());

    if (f.bad())
    {
      throw runtime_error("Can't access HTML file.");
    }

    HtmlRender render(htmlText);
    render.Rend();
  }
  catch (const exception& exc)
  {
    cout << "Error: " << exc.what() << endl;
    return 1;
  }

  return 0;
}