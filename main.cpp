#define _WIN32_WINNT 0x0A00    
#define HTTPLIB_IMPLEMENTATION 
#include "httplib.h"           

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "json.hpp" 

using json = nlohmann::json;
using namespace httplib;

const std::string DB_FILE = "posts.json";



/**
 * @class Comment
 * @brief Represents a single comment on a post.
 */
class Comment
{
public:
    std::string author;
    std::string content;
    std::string timestamp;

    Comment(const std::string &author, const std::string &content)
        : author(author), content(content)
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        timestamp = ss.str();
    }

    Comment(const std::string &author, const std::string &content, const std::string &timestamp)
        : author(author), content(content), timestamp(timestamp) {}

    json toJson() const
    {
        return {
            {"author", author},
            {"content", content},
            {"timestamp", timestamp}};
    }
};

/**
 * @class Post (Base Class)
 * @brief Represents a single post on the platform.
 *
 */
class Post
{
protected:
    long long id;
    std::string author;
    std::string content;
    std::string timestamp;
    int likes;
    std::vector<Comment> comments; 

public:
    Post(long long id, const std::string &author, const std::string &content)
        : id(id), author(author), content(content), likes(0)
    {
        // Generate current timestamp
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        timestamp = ss.str();
    }

    virtual ~Post() {}

    virtual std::string getType() const = 0; 

    virtual json toJson() const
    {
        json j;
        j["id"] = id;
        j["author"] = author;
        j["content"] = content;
        j["timestamp"] = timestamp;
        j["likes"] = likes;
        j["type"] = getType(); 

        json j_comments = json::array();
        for (const auto &comment : comments)
        {
            j_comments.push_back(comment.toJson());
        }
        j["comments"] = j_comments;

        return j;
    }

    virtual Comment &addComment(const std::string &author, const std::string &content)
    {
        comments.emplace_back(author, content);
        return comments.back();
    }

    void loadComment(const Comment &comment)
    {
        comments.push_back(comment);
    }

    void setTimestamp(const std::string &ts)
    {
        timestamp = ts;
    }

    void incrementLikes()
    {
        likes++;
    }

    void decrementLikes()
    {
        if (likes > 0)
        { 
            likes--;
        }
    }

    void setLikes(int numLikes)
    {
        likes = numLikes;
    }

    long long getId() const { return id; }
    std::string getAuthor() const { return author; }
    std::string getContent() const { return content; }
};

/**
 * @class GeneralPost
 * @brief Represents a general discussion post.
 * Demonstrates Inheritance.
 */
class GeneralPost : public Post
{
public:
    GeneralPost(long long id, const std::string &author, const std::string &content)
        : Post(id, author, content) {}

    std::string getType() const override
    {
        return "general";
    }
};

/**
 * @class LostAndFoundPost
 * @brief Represents a post for lost or found items.
 * Demonstrates Inheritance and adds specific properties.
 */
class LostAndFoundPost : public Post
{
private:
    std::string itemStatus; 

public:
    LostAndFoundPost(long long id, const std::string &author, const std::string &content, const std::string &status)
        : Post(id, author, content), itemStatus(status) {}

    std::string getType() const override
    {
        return itemStatus;
    }

    json toJson() const override
    {
        json j = Post::toJson();      
        j["itemStatus"] = itemStatus; 
        return j;
    }
};

/**
 * @class HelpRequestPost
 * @brief Represents a post asking for help.
 * Demonstrates Inheritance.
 */
class HelpRequestPost : public Post
{
public:
    HelpRequestPost(long long id, const std::string &author, const std::string &content)
        : Post(id, author, content) {}

    std::string getType() const override
    {
        return "help";
    }
};

/**
 * @class EventsPost
 * @brief Represents a post about an event.
 * DemonstrD/ates Inheritance.
 */
class EventsPost : public Post
{
public:
    EventsPost(long long id, const std::string &author, const std::string &content)
        : Post(id, author, content) {}

    std::string getType() const override
    {
        return "events";
    }
};

/**
 * @class AcademicPost
 * @brief Represents an academic-related post.
 * Demonstrates Inheritance.
 */
class AcademicPost : public Post
{
public:
    AcademicPost(long long id, const std::string &author, const std::string &content)
        : Post(id, author, content) {}

    std::string getType() const override
    {
        return "academic";
    }
};


/**
 * @class PostService
 * @brief Manages the collection of all posts.
 *
 * This class encapsulates all logic for creating, storing, loading, and
 * retrieving posts, hiding the implementation details from the server.
 */
class PostService
{
private:
    std::vector<std::unique_ptr<Post>> posts;
    long long nextId = 1;

    void updateNextId()
    {
        long long maxId = 0;
        for (const auto &post : posts)
        {
            if (post->getId() > maxId)
            {
                maxId = post->getId();
            }
        }
        nextId = maxId + 1;
    }

public:
    PostService()
    {
        loadPosts(); 
    }

    /**
     * @brief Creates and adds a new post.
     * This is a Factory Method pattern.
     */
    Post *createPost(const std::string &author, const std::string &content, const std::string &type)
    {
        long long id = nextId++;
        std::unique_ptr<Post> newPost;

        if (type == "lost")
        {
            newPost = std::make_unique<LostAndFoundPost>(id, author, content, "lost");
        }
        else if (type == "found")
        {
            newPost = std::make_unique<LostAndFoundPost>(id, author, content, "found");
        }
        else if (type == "help")
        {
            newPost = std::make_unique<HelpRequestPost>(id, author, content);
        }
        else if (type == "events")
        {
            newPost = std::make_unique<EventsPost>(id, author, content);
        }
        else if (type == "academic")
        {
            newPost = std::make_unique<AcademicPost>(id, author, content);
        }
        else
        {
            newPost = std::make_unique<GeneralPost>(id, author, content);
        }

        Post *newPostPtr = newPost.get();
        posts.push_back(std::move(newPost));
        savePosts(); 
        return newPostPtr;
    }

    /**
     * @brief Adds a comment to a specific post by its ID.
     */
    Comment *addCommentToPost(long long postId, const std::string &author, const std::string &content)
    {
        for (auto &post : posts)
        {
            if (post->getId() == postId)
            {
                Comment &newComment = post->addComment(author, content);
                savePosts();
                return &newComment;
            }
        }
        return nullptr;
    }

    /**
     * @brief Increments the like count for a post.
     */
    Post *likePost(long long postId)
    {
        for (auto &post : posts)
        {
            if (post->getId() == postId)
            {
                post->incrementLikes();
                savePosts(); 
                return post.get();
            }
        }
        return nullptr; // Post not found
    }

    /**
     * @brief --- NEW: Decrements the like count for a post ---
     */
    Post *unlikePost(long long postId)
    {
        for (auto &post : posts)
        {
            if (post->getId() == postId)
            {
                post->decrementLikes();
                savePosts(); // Save after updating likes
                return post.get();
            }
        }
        return nullptr; // Post not found
    }

    /**
     * @brief Serializes all posts to a JSON array.
     */
    json getAllPostsAsJson() const
    {
        json j = json::array();
        // Iterate in reverse to show newest first
        for (auto it = posts.rbegin(); it != posts.rend(); ++it)
        {
            j.push_back((*it)->toJson());
        }
        return j;
    }

    /**
     * @brief Saves all posts to the JSON database file.
     */
    void savePosts() const
    {
        std::ofstream f(DB_FILE);
        if (f.is_open())
        {
            f << getAllPostsAsJson().dump(4); 
            f.close();
            std::cout << "Saved " << posts.size() << " posts to " << DB_FILE << std::endl;
        }
        else
        {
            std::cerr << "Error: Could not open " << DB_FILE << " for writing." << std::endl;
        }
    }

    /**
     * @brief Loads all posts from the JSON database file.
     */
    void loadPosts()
    {
        std::ifstream f(DB_FILE);
        if (!f.is_open())
        {
            std::cerr << "Warning: " << DB_FILE << " not found. Starting fresh." << std::endl;
            return;
        }

        json j;
        try
        {
            f >> j;
        }
        catch (json::parse_error &e)
        {
            std::cerr << "Error: Could not parse " << DB_FILE << ". " << e.what() << std::endl;
            f.close();
            return;
        }
        f.close();

        posts.clear();
        for (const auto &item : j)
        {
            try
            {
                std::string type = item.at("type");
                long long id = item.at("id");
                std::string author = item.at("author");
                std::string content = item.at("content");
                std::string timestamp = item.at("timestamp"); 

                int likes = 0;
                if (item.contains("likes"))
                {
                    likes = item.at("likes");
                }

                std::unique_ptr<Post> newPost;

                if (type == "lost")
                {
                    newPost = std::make_unique<LostAndFoundPost>(id, author, content, "lost");
                }
                else if (type == "found")
                {
                    newPost = std::make_unique<LostAndFoundPost>(id, author, content, "found");
                }
                else if (type == "help")
                {
                    newPost = std::make_unique<HelpRequestPost>(id, author, content);
                }
                else if (type == "events")
                {
                    newPost = std::make_unique<EventsPost>(id, author, content);
                }
                else if (type == "academic")
                {
                    newPost = std::make_unique<AcademicPost>(id, author, content);
                }
                else
                {
                    
                    newPost = std::make_unique<GeneralPost>(id, author, content);
                }

                if (newPost)
                {
                    newPost->setTimestamp(timestamp); 
                    newPost->setLikes(likes);         

                    if (item.contains("comments") && item.at("comments").is_array())
                    {
                        for (const auto &c_item : item.at("comments"))
                        {
                            newPost->loadComment(Comment(
                                c_item.at("author"),
                                c_item.at("content"),
                                c_item.at("timestamp")));
                        }
                    }
                    posts.push_back(std::move(newPost));
                }
            }
            catch (json::exception &e)
            {
                std::cerr << "Error: Skipping malformed post in JSON. " << e.what() << std::endl;
            }
        }
        updateNextId(); 
        std::cout << "Loaded " << posts.size() << " posts from " << DB_FILE << std::endl;
    }
};



int main()
{
    
    Server svr;

    PostService postService;

   
    svr.Get("/api/posts", [&](const Request &req, Response &res)
            {
        json postsJson = postService.getAllPostsAsJson();
        res.set_content(postsJson.dump(), "application/json"); });

    svr.Post("/api/posts", [&](const Request &req, Response &res)
             {
        try {
            json newPostData = json::parse(req.body);
            std::string author = newPostData.at("author");
            std::string content = newPostData.at("content");
            std::string type = newPostData.at("type");

            if (author.empty() || content.empty()) {
                throw std::runtime_error("Author and content cannot be empty.");
            }
            Post* newPost = postService.createPost(author, content, type);
            res.set_content(newPost->toJson().dump(), "application/json");

        } catch (json::exception& e) {
            res.status = 400; // Bad Request
            json err = {{"error", "Invalid JSON data"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        } catch (std::exception& e) {
            res.status = 400; // Bad Request
            json err = {{"error", "Invalid post data"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        } });

   
    svr.Post(R"(/api/posts/(\d+)/comments)", [&](const Request &req, Response &res)
             {
        long long postId = std::stoll(req.matches[1]);
        try {
            json commentData = json::parse(req.body);
            std::string author = commentData.at("author");
            std::string content = commentData.at("content");

            if (author.empty() || content.empty()) {
                throw std::runtime_error("Author and content cannot be empty.");
            }
            Comment* newComment = postService.addCommentToPost(postId, author, content);
            if (newComment) {
                res.set_content(newComment->toJson().dump(), "application/json");
            } else {
                res.status = 404; // Not Found
                json err = {{"error", "Post not found"}};
                res.set_content(err.dump(), "application/json");
            }
        } catch (json::exception& e) {
            res.status = 400; // Bad Request
            json err = {{"error", "Invalid JSON data"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        } catch (std::exception& e) {
            res.status = 400; // Bad Request
            json err = {{"error", "Invalid comment data"}, {"detail", e.what()}};
            res.set_content(err.dump(), "application/json");
        } });

    svr.Post(R"(/api/posts/(\d+)/like)", [&](const Request &req, Response &res)
             {
        long long postId = std::stoll(req.matches[1]);
        
        Post* updatedPost = postService.likePost(postId);
        
        if (updatedPost) {
            res.set_content(updatedPost->toJson().dump(), "application/json");
        } else {
            res.status = 404; // Not Found
            json err = {{"error", "Post not found"}};
            res.set_content(err.dump(), "application/json");
        } });

    svr.Post(R"(/api/posts/(\d+)/unlike)", [&](const Request &req, Response &res)
             {
        long long postId = std::stoll(req.matches[1]);
        
        Post* updatedPost = postService.unlikePost(postId);
        
        if (updatedPost) {
            res.set_content(updatedPost->toJson().dump(), "application/json");
        } else {
            res.status = 404; // Not Found
            json err = {{"error", "Post not found"}};
            res.set_content(err.dump(), "application/json");
        } });

    auto ret = svr.set_base_dir("./www");
    if (!ret)
    {
        std::cerr << "Error: Could not set base directory './www'.\n";
        std::cerr << "Make sure the 'www' directory exists and contains index.html.\n";
        return 1;
    }

    std::cout << "DTU Connect server starting at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}