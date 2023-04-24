#ifndef RIVER_BUILDER_HPP
#define RIVER_BUILDER_HPP

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "channel.hpp"
#include "link.hpp"
#include "lock.hpp"
#include "river.hpp"
#include "rivulet.hpp"

namespace river {
/**
 * Builder class for rivers.
 */
class Builder final {
public:
    /**
     * Error codes that the builder interface can return.
     * @{
     */
    static constexpr int32_t ERR_INVALID = 1;
    static constexpr int32_t ERR_NOTFOUND = 2;
    static constexpr int32_t ERR_DUPE = 3;
    static constexpr int32_t ERR_NOTROOT = 4;
    /**
     * @}
     */

    /**
     * Default constructor.
     */
    Builder();

    /**
     * Adds a channel to the river.
     *
     * Reading or writing the channel will not have any effect until the river
     * is built with Builder::build().
     *
     * @tparam T Channel type. This type should be fixed-size, copy-
     *           constructible, and should not contain pointers.
     *
     * @param      path     Channel path.
     * @param      init_val Channel initial value.
     * @param[out] channel  On success, handle to added channel.
     *
     * @retval 0           Success.
     * @retval ERR_INVALID Path is invalid.
     * @retval ERR_DUPE    Channel at path already exists.
     */
    template <typename T>
    int32_t channel(const std::string& path,
                    const T init_val,
                    Channel<T>& channel)
    {
        static_assert(std::is_copy_constructible<T>::value);

        // Tokenize the path.
        std::vector<std::string> tokens;
        const int32_t tokenize_ret = tokenize_path(path, tokens);
        if (tokenize_ret != 0) {
            return tokenize_ret;
        }

        // Add node for the channel to the metadata tree.
        std::shared_ptr<Node> channel_node;
        insert_node(root,
                    tokens,
                    /* index= */ 0,
                    /* create= */ true,
                    channel_node);

        // Check that a channel at this path doesn't already exist.
        if (channel_node->channel_info) {
            return ERR_DUPE;
        }

        // Set info for new channel node.
        channel_node->name = tokens.back();
        channel_node->channel_info.reset(new ChannelInfo<T>(init_val));

        // Link the returned channel handle to the river. Note that the channel
        // node can already have a link if there's also a rivulet at this path.
        if (!channel_node->link) {
            channel_node->link.reset(new Link);
        }
        channel.link = channel_node->link;

        return 0;
    }

    /**
     * Gets a handle to a rivulet.
     *
     * Reading or writing the rivulet will not have any effect until the river
     * is built with Builder::build().
     *
     * @param      path    Rivulet path.
     * @param[out] rivulet On success, handle to rivulet.
     *
     * @retval 0            Success.
     * @retval ERR_INVALID  Path is invalid.
     * @retval ERR_NOTFOUND Path doesn't exist.
     */
    int32_t rivulet(const std::string& path, Rivulet& rivulet);

    /**
     * Adds a lock to a rivulet.
     *
     * @param path Rivulet path.
     * @param lock Lock to use.
     *
     * @retval 0            Success.
     * @retval ERR_INVALID  Path is invalid.
     * @retval ERR_NOTFOUND Path doesn't exist.
     * @retval ERR_DUPE     Path is already locked.
     */
    int32_t lock(const std::string& path, const std::shared_ptr<Lock> lock);

    /**
     * Builds the river.
     *
     * All channel and rivulet handles produced by the builder will become valid
     * and can be used to read and write the river.
     *
     * After building, the builder retains the structure metadata for the built
     * river, so that the caller can continue building on the structure and
     * build additional rivers from the same builder. However, additional built
     * rivers will be completely disjoint from ones built previously.
     *
     * The caller is not required to save the River returned by this method. All
     * channel and rivulet handles have shared pointers to the river, so that
     * the river exists as long as at least one handle to it also exists.
     *
     * @param[out] river_ret If not null, will be populated with a pointer to
     *                       the built river.
     *
     * @retval 0           Success.
     * @retval ERR_NOTROOT Builder is not the root builder for the river.
     */
    int32_t build(std::shared_ptr<River>* const river_ret);

    /**
     * Builds the river, opting not to save the returned pointer.
     *
     * @see Builder::build(std::shared_ptr<River>*)
     */
    int32_t build();

    /**
     * Gets a builder rooted at the specified path, a.k.a. a rivulet builder.
     *
     * The rivulet builder is for the exact same river, but paths passed to it
     * are relative to the rivulet path. This is similar to a `cd` operation in
     * a file system.
     *
     * Rivulet builders cannot be used to `build()` the larger river.
     *
     * @param      path    Rivulet path.
     * @param[out] builder On success, builder for rivulet.
     *
     * @retval 0           Success.
     * @retval ERR_INVALID Path is invalid.
     */
    int32_t sub(const std::string& path, Builder& builder);

private:
    /**
     * Base class for ChannelInfo<T> instantiations.
     */
    struct ChannelInfoBase {
        /**
         * Destructor.
         */
        virtual ~ChannelInfoBase() = default;

        /**
         * @see ChannelInfo<T>::init_val_addr()
         */
        virtual const void* init_val_addr() const = 0;

        /**
         * @see ChannelInfo<T>::size()
         */
        virtual size_t size() const = 0;
    };

    /**
     * Holds metadata about a channel in the river.
     *
     * @tparam T Channel type.
     */
    template <typename T>
    struct ChannelInfo final : public ChannelInfoBase {
    public:
        /**
         * Constructor.
         *
         * @param init_val_ Channel initial value.
         */
        ChannelInfo(const T init_val_)
            : init_val(init_val_)
        {
        }

        /**
         * Gets the address of the channel initial value.
         *
         * @returns Initial value address.
         */
        const void* init_val_addr() const override
        {
            return &init_val;
        }

        /**
         * Gets the size of the channel type in bytes.
         *
         * @returns Channel type size in bytes.
         */
        size_t size() const override
        {
            return sizeof(T);
        }

    private:
        /**
         * Channel initial value.
         */
        const T init_val;
    };

    /**
     * A node in the river metadata tree.
     */
    struct Node final {
        /**
         * Name of this node.
         *
         * This is a token in a path, e.g., `bar` in `foo.bar.baz`.
         */
        std::string name;

        /**
         * If this node represents a channel, points to channel info.
         */
        std::shared_ptr<ChannelInfoBase> channel_info;

        /**
         * River link for the channel or rivulet represented by this node.
         */
        std::shared_ptr<Link> link;

        /**
         * Child nodes.
         */
        std::vector<std::shared_ptr<Node>> children;
    };

    /**
     * River metadata tree root.
     */
    std::shared_ptr<Node> root;

    /**
     * Whether this is the root builder for the river.
     */
    bool is_root;

    /**
     * Tokenizes a path.
     *
     * @param      path   Path to tokenize.
     * @param[out] tokens On success, contains path tokens.
     *
     * @retval 0           Path is valid.
     * @retval ERR_INVALID Path is invalid.
     */
    static int32_t tokenize_path(const std::string& path,
                                 std::vector<std::string>& tokens);

    /**
     * Constructor for a rivulet builder.
     *
     * @param root Builder root node.
     */
    explicit Builder(const std::shared_ptr<Node> root);

    /**
     * Inserts a node into the river metadata tree.
     *
     * @param      node     Current node in the recursion.
     * @param      path     Path to insert node at.
     * @param      index    Current token index in path.
     * @param      create   If true, a new node will be created at the path if
     *                      it doesn't already exist.
     * @param[out] node_ret If the node already exists, points to the node.
     */
    void insert_node(const std::shared_ptr<Node> node,
                     const std::vector<std::string>& path,
                     const size_t index,
                     const bool create,
                     std::shared_ptr<Node>& node_ret);

    /**
     * Recursive helper that builds the rivulet rooted at a node.
     *
     * @param node  Current node in the recursion.
     * @param river River being built.
     */
    void build_node(const std::shared_ptr<Node> node,
                    const std::shared_ptr<River> river);

    /**
     * Executes a function for each node in the river metadata tree.
     *
     * @param node Current node in the recursion.
     * @param func Function to execute.
     */
    int32_t for_each_node(const std::shared_ptr<Node> node,
                          const std::function<int32_t(
                              const std::shared_ptr<Node>)>
                              func);

    /**
     * Pretty-prints the river metadata.
     *
     * @param os      Output stream.
     * @param builder Builder to print.
     */
    friend std::ostream& operator<<(std::ostream& os,
                                    const Builder& builder);

    /**
     * Print helper function.
     *
     * @param os           Output stream.
     * @param node         Current node in the recursion.
     * @param root         Whether this node is the root node.
     * @param indent_level Current indent level in the printed hierarchy.
     *
     * @returns Output stream.
     */
    static std::ostream& print_helper(std::ostream& os,
                                      const std::shared_ptr<Node> node,
                                      const bool root,
                                      const uint64_t indent_level);
};
} /* namespace river */

#endif
