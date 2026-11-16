// Unit tests for themis::query::RequestContext (deadline + cancellation token).

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "query/request_context.h"

using namespace themis::query;
using namespace std::chrono_literals;

TEST(RequestContextTest, DefaultIsUnboundedAndNotCancellable) {
    RequestContext ctx;
    EXPECT_FALSE(ctx.hasDeadline());
    EXPECT_FALSE(ctx.remaining().has_value());
    EXPECT_EQ(ctx.stopReason(), StopReason::None);
    EXPECT_FALSE(ctx.shouldStop());
}

TEST(RequestContextTest, CancellationIsReported) {
    auto token = std::make_shared<QueryCancellationToken>();
    RequestContext ctx(token);
    EXPECT_EQ(ctx.stopReason(), StopReason::None);
    token->cancel();
    EXPECT_EQ(ctx.stopReason(), StopReason::Cancelled);
}

TEST(RequestContextTest, DeadlineExpiryIsDistinctFromCancellation) {
    auto live = RequestContext::withTimeout(30s);
    EXPECT_EQ(live.stopReason(), StopReason::None);
    EXPECT_GT(live.remaining()->count(), 0);

    auto expired = RequestContext::withTimeout(-1ms);
    EXPECT_EQ(expired.stopReason(), StopReason::DeadlineExceeded);
    EXPECT_EQ(expired.remaining()->count(), 0);
}

TEST(RequestContextTest, CancellationTakesPrecedenceOverDeadline) {
    auto token = std::make_shared<QueryCancellationToken>();
    auto ctx = RequestContext::withTimeout(-1ms, token);
    token->cancel();
    EXPECT_EQ(ctx.stopReason(), StopReason::Cancelled);
}

TEST(RequestContextTest, ChildClampsDeadlineAndSharesToken) {
    auto token = std::make_shared<QueryCancellationToken>();
    auto parent = RequestContext::withTimeout(50ms, token);

    auto tighter = parent.child(10ms);
    EXPECT_LE(*tighter.deadline(), *parent.deadline());

    auto looser = parent.child(10s);  // cannot extend the parent's deadline
    EXPECT_EQ(*looser.deadline(), *parent.deadline());

    token->cancel();
    EXPECT_EQ(tighter.stopReason(), StopReason::Cancelled);
    EXPECT_EQ(looser.stopReason(), StopReason::Cancelled);
}

TEST(RequestContextTest, ChildOfUnboundedParentGetsDeadline) {
    RequestContext parent;
    auto child = parent.child(20s);
    EXPECT_TRUE(child.hasDeadline());
    EXPECT_EQ(child.stopReason(), StopReason::None);
}
