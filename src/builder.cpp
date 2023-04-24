#include <cassert>
#include <cctype>
#include <cstring>
#include <sstream>

#include "builder.hpp"

namespace river {
Builder::Builder()
    : root(new Node)
    , is_root(true)
{
}

int32_t Builder::rivulet(const std::string& path, Rivulet& rivulet)
{
    // Tokenize the path.
    std::vector<std::string> tokens;
    const int32_t tokenize_ret = tokenize_path(path, tokens);
    if (tokenize_ret != 0) {
        return tokenize_ret;
    }

    // Get node at the path.
    std::shared_ptr<Node> rivulet_node;
    insert_node(root,
                tokens,
                /* index= */ 0,
                /* create= */ false,
                rivulet_node);

    // Check that the path exists.
    if (!rivulet_node) {
        return ERR_NOTFOUND;
    }

    // Link the returned rivulet handle to the river. Note that the rivulet node
    // can already have a link if there's also a channel at this path.
    if (!rivulet_node->link) {
        rivulet_node->link.reset(new Link);
    }
    rivulet.link = rivulet_node->link;

    return 0;
}

int32_t Builder::lock(const std::string& path,
                      const std::shared_ptr<Lock> lock)
{
    // Tokenize the path.
    std::vector<std::string> tokens;
    const int32_t tokenize_ret = tokenize_path(path, tokens);
    if (tokenize_ret != 0) {
        return tokenize_ret;
    }

    // Get node at the path.
    std::shared_ptr<Node> node;
    insert_node(root,
                tokens,
                /* index= */ 0,
                /* create= */ false,
                node);

    // Check that the path exists.
    if (!node) {
        return ERR_NOTFOUND;
    }

    // Check that no node in this subtree already has a lock.
    static const auto check_for_locks =
        [](const std::shared_ptr<Node> node) -> int32_t {
        assert(node);
        assert(node->link);
        return (node->link->lock ? -1 : 0);
    };
    if (for_each_node(node, check_for_locks)) {
        return ERR_DUPE;
    }

    // Assign lock to all nodes in this subtree.
    const auto assign_lock =
        [&lock](const std::shared_ptr<Node> node) -> int32_t {
        assert(node);
        assert(node->link);
        assert(!node->link->lock);
        node->link->lock = lock;
        return 0;
    };
    for_each_node(node, assign_lock);

    return 0;
}

int32_t Builder::build(std::shared_ptr<River>* river_ret)
{
    // Check that this is the root builder.
    if (!is_root) {
        return ERR_NOTROOT;
    }

    std::shared_ptr<River> river(new River);
    build_node(root, river);

    // Remove all river links from the metadata tree so that any future rivers
    // built by this builder don't link to the one we just built.
    static const auto remove_link =
        [](const std::shared_ptr<Node> node) -> int32_t {
        node->link.reset();
        return 0;
    };
    for_each_node(root, remove_link);

    // Return river.
    if (river_ret) {
        *river_ret = river;
    }

    return 0;
}

int32_t Builder::build()
{
    return build(nullptr);
}

int32_t Builder::sub(const std::string& path, Builder& builder)
{
    // Tokenize the path.
    std::vector<std::string> tokens;
    const int32_t tokenize_ret = tokenize_path(path, tokens);
    if (tokenize_ret != 0) {
        return tokenize_ret;
    }

    // Get node at the path.
    std::shared_ptr<Node> node;
    insert_node(root,
                tokens,
                /* index= */ 0,
                /* create= */ true,
                node);

    // Return builder rooted at the path.
    builder = Builder(node);

    return 0;
}

int32_t Builder::tokenize_path(const std::string& path,
                               std::vector<std::string>& tokens)
{
    std::stringstream ss(path);

    // Split path on dots.
    std::string token;
    while (std::getline(ss, token, '.')) {
        tokens.push_back(token);
    }

    // Check that path is non-empty.
    if (tokens.empty()) {
        return ERR_INVALID;
    }

    // Check that each token is non-empty and contains only valid characters.
    for (const std::string& token : tokens) {
        if (token.empty()) {
            return ERR_INVALID;
        }

        // Valid characters are letters, numbers, underscores, and dots.
        static const auto char_is_invalid =
            [](const char c) -> bool {
            return (!std::isalnum(c) && c != '_' && c != '.');
        };
        if (std::find_if(token.begin(), token.end(), char_is_invalid)
            != token.end()) {
            return ERR_INVALID;
        }
    }

    return 0;
}

Builder::Builder(const std::shared_ptr<Node> root_)
    : root(root_)
    , is_root(false)
{
}

void Builder::insert_node(const std::shared_ptr<Node> node,
                          const std::vector<std::string>& path,
                          const size_t index,
                          const bool create,
                          std::shared_ptr<Node>& node_ret)
{
    assert(node);
    assert(index <= path.size());

    // Base case; fell off the path.
    if (index == path.size()) {
        node_ret = node;
        return;
    }

    // Search for a child node matching the current path token.
    const std::string& token = path[index];
    for (const std::shared_ptr<Node>& child : node->children) {
        // Found matching child node; recurse into it.
        if (child->name == token) {
            insert_node(child, path, index + 1, create, node_ret);
            return;
        }
    }

    // Node at specified path doesn't exist. If we didn't intend to create a new
    // node, stop here.
    if (!create) {
        return;
    }

    // Create a new node at this path and recurse into it.
    const std::shared_ptr<Node> new_child(new Node {
        .name = token,
        .channel_info = nullptr,
        .link = nullptr,
        .children = {},
    });
    node->children.push_back(new_child);
    insert_node(new_child, path, index + 1, create, node_ret);
}

void Builder::build_node(const std::shared_ptr<Node> node,
                         const std::shared_ptr<River> river)
{
    assert(river);

    // Do nothing if node is null. This is only possible when the river is
    // empty.
    if (!node) {
        return;
    }

    // Establish the link to the river. This is the link held by any channel or
    // rivulet handles represented by this node.
    const auto& link = node->link;
    if (link) {
        link->river = river;

        // If channel info is present, this node represents a channel; add it
        // to the river.
        const auto& channel_info = node->channel_info;
        if (channel_info) {
            // Increase size of river to fit the new channel at the end.
            const size_t old_river_size = river->storage->size();
            river->storage->resize(old_river_size + channel_info->size());

            // Set the channel size in its link.
            link->channel_offset = old_river_size;

            // Copy initial channel value to river.
            uint8_t* const river_addr = river->storage->data();
            std::memcpy(river_addr + link->channel_offset,
                        channel_info->init_val_addr(),
                        channel_info->size());
        }
    }

    // Recurse into node's children.
    for (const std::shared_ptr<Node>& child : node->children) {
        build_node(child, river);
    }

    // Compute the size and offset of the rivulet rooted at this node. It's
    // important that this happens after recursing into the node's children, so
    // that the offset of the first child (and thus the rivulet) is known.
    if (link) {
        size_t rivulet_size = 0;
        const auto sum_size =
            [&rivulet_size](const std::shared_ptr<Node> node) -> int32_t {
            assert(node);
            if (node->channel_info) {
                rivulet_size += node->channel_info->size();
            }
            return 0;
        };
        for (const std::shared_ptr<Node>& child : node->children) {
            for_each_node(child, sum_size);
        }

        // Set the rivulet size and offset. The offset is the offset of the
        // first channel within the rivulet.
        link->rivulet_size = rivulet_size;
        link->rivulet_offset = node->children.empty()
            ? 0
            : node->children[0]->link->channel_offset;
    }
}

int32_t Builder::for_each_node(const std::shared_ptr<Node> node,
                               const std::function<int32_t(
                                   const std::shared_ptr<Node>)>
                                   func)
{
    assert(node);

    // Call the function with the node, bubbling up the error if there is one.
    int32_t ret = func(node);
    if (ret) {
        return ret;
    }

    // Recurse into node's children.
    for (const std::shared_ptr<Node>& child : node->children) {
        ret = for_each_node(child, func);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

std::ostream& operator<<(std::ostream& os, const Builder& builder)
{
    return Builder::print_helper(os,
                                 builder.root,
                                 /* root= */ true,
                                 /* indent_level= */ 0);
}

std::ostream& Builder::print_helper(std::ostream& os,
                                    const std::shared_ptr<Node> node,
                                    const bool root,
                                    const uint64_t indent_level)
{
    assert(node);

    // Don't print anything if the rivulet is empty.
    if (node->name.empty() && node->children.empty()) {
        return os;
    }

    // Print indents.
    std::ostream* ret = &os;
    for (uint64_t i = 0; i < indent_level; ++i) {
        ret = &(*ret << "    ");
    }

    // Print node name if it isn't the root node.
    if (!root) {
        ret = &(*ret << node->name << "\n");
    }

    // Print rivulets.
    for (const std::shared_ptr<Node>& child : node->children) {
        ret = &print_helper(*ret,
                            child,
                            /* root= */ false,
                            indent_level + !root);
    }

    return *ret;
}
} /* namespace river */
