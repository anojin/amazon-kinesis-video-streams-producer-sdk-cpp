#include "ViewTestFixture.h"

class ViewApiFunctionalityTest : public ViewTestBase {
};

PContentView gContentView = NULL;
UINT64 gCustomData = 0;
ViewItem gViewItem;
BOOL gCurrent = FALSE;
UINT32 gCallCount = 0;

#define MAX_VIEW_ITERATION_COUNT 50
#define VIEW_ITEM_DURATION 10
#define VIEW_ITEM_DURATION_LARGE 20

#define VIEW_ITEM_ALLOCAITON_SIZE 100

TEST_F(ViewApiFunctionalityTest, SimpleAddGet)
{
    UINT64 index, curIndex;
    BOOL itemExists, currentAvailability, windowAvailability;
    PViewItem pViewItem;
    UINT64 timestamp = 0, currentDuration = 0, windowDuration = 0;
    UINT64 currentSize = 0, windowSize = 0;
    UINT64 currentAllocationSize = 0, windowAllocationSize = 0;

    CreateContentView();

    // Add/check
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        currentAvailability = windowAvailability = FALSE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
        EXPECT_TRUE(currentAvailability);
        EXPECT_TRUE(windowAvailability);
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(0, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(0, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    }

    currentAvailability = windowAvailability = FALSE;
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_TRUE(currentAvailability);
    EXPECT_TRUE(windowAvailability);

    // Get next/iterate
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        // Get the current and window durations
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowDuration(mContentView, &currentDuration, &windowDuration));
        EXPECT_EQ((MAX_VIEW_ITERATION_COUNT - 1) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, windowDuration);
        EXPECT_EQ((MAX_VIEW_ITERATION_COUNT - index - 1) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, currentDuration);

        // Get the current and window sizes
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowItemCount(mContentView, &currentSize, &windowSize));
        EXPECT_EQ(MAX_VIEW_ITERATION_COUNT, windowSize);
        EXPECT_EQ(MAX_VIEW_ITERATION_COUNT - index, currentSize);

        // Get the current and window allocation size
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowAllocationSize(mContentView, &currentAllocationSize, &windowAllocationSize));
        EXPECT_EQ(MAX_VIEW_ITERATION_COUNT * VIEW_ITEM_ALLOCAITON_SIZE, windowAllocationSize);
        EXPECT_EQ((MAX_VIEW_ITERATION_COUNT - index) * VIEW_ITEM_ALLOCAITON_SIZE, currentAllocationSize);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(index, pViewItem->index);
    }

    currentAvailability = windowAvailability = FALSE;
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_TRUE(currentAvailability);
    EXPECT_TRUE(windowAvailability);

    // Get the current and window durations
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowDuration(mContentView, &currentDuration, &windowDuration));
    EXPECT_EQ((MAX_VIEW_ITERATION_COUNT - 1) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, windowDuration);
    EXPECT_EQ(0, currentDuration);

    // Get the current and window sizes
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowItemCount(mContentView, &currentSize, &windowSize));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT, windowSize);
    EXPECT_EQ(0, currentSize);

    // Get the current and window allocation sizes
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowAllocationSize(mContentView, &currentAllocationSize, &windowAllocationSize));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT * VIEW_ITEM_ALLOCAITON_SIZE, windowAllocationSize);
    EXPECT_EQ(0, currentAllocationSize);

    // should have no more items
    EXPECT_EQ(STATUS_CONTENT_VIEW_NO_MORE_ITEMS, contentViewGetNext(mContentView, &pViewItem));

    // Reset and try again
    EXPECT_EQ(STATUS_SUCCESS, contentViewResetCurrent(mContentView));

    // Get next/iterate
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(index, pViewItem->index);
    }
}

TEST_F(ViewApiFunctionalityTest, AddGetSetCurrent)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp = 0;

    CreateContentView();

    // Add/check
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION + 1) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(0, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);

        // Get and Set the current again
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, curIndex));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);
    }
}

TEST_F(ViewApiFunctionalityTest, AddGetSetCurrentRemoveAll)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp = 0;
    UINT64 curDuration, windowDuration;
    UINT64 curItemCount, windowItemCount;

    CreateContentView();

    // Add/check
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION + 1) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(0, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));
        EXPECT_EQ(timestamp, pViewItem->timestamp);
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(index, pViewItem->index);

        // Get and Set the current again
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, curIndex));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(index + 1, curIndex);
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITERATION_COUNT / 2));

    // Remove all
    EXPECT_EQ(STATUS_SUCCESS, contentViewRemoveAll(mContentView));

    // Verify that all are removed
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowAllocationSize(mContentView, &curDuration, &windowDuration));
    EXPECT_EQ(0, curDuration);
    EXPECT_EQ(0, windowDuration);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowItemCount(mContentView, &curItemCount, &windowItemCount));
    EXPECT_EQ(0, curItemCount);
    EXPECT_EQ(0, windowItemCount);

    // Verify the call count
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT, gCallCount);
}

TEST_F(ViewApiFunctionalityTest, OverflowCheck)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp = 0, currentDuration = 0, windowDuration = 0, currentAllocationSize = 0, windowAllocationSize = 0;
    UINT64 currentSize = 0, windowSize = 0;

    CreateContentView();

    // Add/check
    for (timestamp = 0, index = 0; index < 2 * MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        // Get the tail
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));

        // Get the current and window durations
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowDuration(mContentView, &currentDuration, &windowDuration));

        // Get the current and window sizes
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowItemCount(mContentView, &currentSize, &windowSize));

        // Get the current and window allocation sizes
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetWindowAllocationSize(mContentView, &currentAllocationSize, &windowAllocationSize));

        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        if (index < MAX_VIEW_ITEM_COUNT) {
            EXPECT_EQ(0, pViewItem->timestamp);
            EXPECT_EQ(0, pViewItem->index);
            EXPECT_EQ(index * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, windowDuration);
            EXPECT_EQ(index * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, currentDuration);
            EXPECT_EQ(index + 1, windowSize);
            EXPECT_EQ(index + 1, currentSize);
            EXPECT_EQ((index + 1) * VIEW_ITEM_ALLOCAITON_SIZE, windowAllocationSize);
            EXPECT_EQ((index + 1) * VIEW_ITEM_ALLOCAITON_SIZE, currentAllocationSize);
        } else {
            EXPECT_EQ((index % MAX_VIEW_ITEM_COUNT) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, pViewItem->timestamp);
            EXPECT_EQ(index % MAX_VIEW_ITEM_COUNT + 1, pViewItem->index);
            EXPECT_EQ((MAX_VIEW_ITEM_COUNT - 1) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, windowDuration);
            EXPECT_EQ((MAX_VIEW_ITEM_COUNT - 1) * VIEW_ITEM_DURATION + VIEW_ITEM_DURATION, currentDuration);
            EXPECT_EQ(MAX_VIEW_ITEM_COUNT, windowSize);
            EXPECT_EQ(MAX_VIEW_ITEM_COUNT, currentSize);
            EXPECT_EQ(MAX_VIEW_ITEM_COUNT * VIEW_ITEM_ALLOCAITON_SIZE, windowAllocationSize);
            EXPECT_EQ(MAX_VIEW_ITEM_COUNT * VIEW_ITEM_ALLOCAITON_SIZE, currentAllocationSize);
        }

        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        if (index < MAX_VIEW_ITEM_COUNT) {
            EXPECT_EQ(0, curIndex);
        } else {
            EXPECT_EQ(index % MAX_VIEW_ITEM_COUNT + 1, curIndex);
        }

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);
    }
}

TEST_F(ViewApiFunctionalityTest, OverflowNotificationCallbackSizeCurrent)
{
    UINT32 index;
    UINT64 timestamp;
    BOOL currentAvailability, windowAvailability;

    CreateContentView();

    // Add/check overflow on size
    for (timestamp = 0, index = 0; index < 2 * MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        // Check availability before
        EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
        EXPECT_EQ(index < MAX_VIEW_ITEM_COUNT ? TRUE : FALSE, currentAvailability) << "Failed on index: " << index;
        EXPECT_EQ(index < MAX_VIEW_ITEM_COUNT ? TRUE : FALSE, windowAvailability) << "Failed on index: " << index;

        // Add the item
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));

        if (index < MAX_VIEW_ITEM_COUNT) {
            EXPECT_EQ(0, gCustomData);
            EXPECT_EQ(NULL, gContentView);
            EXPECT_EQ(0, gViewItem.timestamp);
            EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ(0, gViewItem.duration);
            EXPECT_EQ(0, gViewItem.handle);
            EXPECT_EQ(0, gViewItem.index);
            EXPECT_FALSE(gCurrent);
        } else {
            EXPECT_EQ(VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA, gCustomData);
            EXPECT_EQ(mContentView, gContentView);
            EXPECT_EQ(index - MAX_VIEW_ITEM_COUNT, gViewItem.index);
            EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, gViewItem.handle);
            EXPECT_EQ(VIEW_ITEM_DURATION, gViewItem.duration);
            EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ((index - MAX_VIEW_ITEM_COUNT) * VIEW_ITEM_DURATION, gViewItem.timestamp);
            EXPECT_TRUE(gCurrent);
        }
    }

    // Set the current and check for availability
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITEM_COUNT + 1));
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_TRUE(currentAvailability);
    EXPECT_FALSE(windowAvailability);
}

TEST_F(ViewApiFunctionalityTest, OverflowNotificationCallbackAvailability)
{
    UINT32 index;
    UINT64 timestamp;
    BOOL currentAvailability, windowAvailability;

    CreateContentView();

    // Add/check overflow on size
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        // Check availability before
        EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
        EXPECT_EQ(index < MAX_VIEW_ITEM_COUNT ? TRUE : FALSE, currentAvailability) << "Failed on index: " << index;
        EXPECT_EQ(index < MAX_VIEW_ITEM_COUNT ? TRUE : FALSE, windowAvailability) << "Failed on index: " << index;

        // Add the item
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(0, gCustomData);
        EXPECT_EQ(NULL, gContentView);
        EXPECT_EQ(0, gViewItem.timestamp);
        EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
        EXPECT_EQ(0, gViewItem.duration);
        EXPECT_EQ(0, gViewItem.handle);
        EXPECT_EQ(0, gViewItem.index);
        EXPECT_FALSE(gCurrent);
    }

    // There should be no availability
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_FALSE(currentAvailability);
    EXPECT_FALSE(windowAvailability);
    EXPECT_EQ(0, gCallCount);

    // Set the current and check for availability
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITEM_COUNT - 1));
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_TRUE(currentAvailability);
    EXPECT_FALSE(windowAvailability);
    EXPECT_EQ(0, gCallCount);

    // Add an item and ensure the callback is called
    EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp + VIEW_ITEM_DURATION, timestamp + VIEW_ITEM_DURATION, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));
    EXPECT_EQ(STATUS_SUCCESS, contentViewCheckAvailability(mContentView, &currentAvailability, &windowAvailability));
    EXPECT_TRUE(currentAvailability);
    EXPECT_FALSE(windowAvailability);
    EXPECT_EQ(1, gCallCount);
}

TEST_F(ViewApiFunctionalityTest, OverflowNotificationCallbackSizeTail)
{
    UINT64 index;
    UINT64 timestamp;
    PViewItem pViewItem;

    CreateContentView();

    // Add/check overflow on size
    for (timestamp = 0, index = 0; index < 2 * MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));

        if (index < MAX_VIEW_ITEM_COUNT) {
            EXPECT_EQ(0, gCustomData);
            EXPECT_EQ(NULL, gContentView);
            EXPECT_EQ(0, gViewItem.timestamp);
            EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ(0, gViewItem.duration);
            EXPECT_EQ(0, gViewItem.handle);
            EXPECT_EQ(0, gViewItem.index);
            EXPECT_FALSE(gCurrent);
        } else {
            EXPECT_EQ(VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA, gCustomData);
            EXPECT_EQ(mContentView, gContentView);
            EXPECT_EQ(index - MAX_VIEW_ITEM_COUNT, gViewItem.index);
            EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, gViewItem.handle);
            EXPECT_EQ(VIEW_ITEM_DURATION, gViewItem.duration);
            EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ((index - MAX_VIEW_ITEM_COUNT) * VIEW_ITEM_DURATION, gViewItem.timestamp);
            EXPECT_FALSE(gCurrent);
        }
    }
}

TEST_F(ViewApiFunctionalityTest, OverflowNotificationCallbackDurationCurrent)
{
    UINT32 index;
    UINT64 timestamp;

    CreateContentView();

    // Add/check overflow on size
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION_LARGE) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION_LARGE, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));

        if (index < MAX_VIEW_ITEM_COUNT / 2) {
            EXPECT_EQ(0, gCustomData);
            EXPECT_EQ(NULL, gContentView);
            EXPECT_EQ(0, gViewItem.timestamp);
            EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ(0, gViewItem.duration);
            EXPECT_EQ(0, gViewItem.handle);
            EXPECT_EQ(0, gViewItem.index);
            EXPECT_FALSE(gCurrent);
        } else {
            EXPECT_EQ(VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA, gCustomData);
            EXPECT_EQ(mContentView, gContentView);
            EXPECT_EQ(index - MAX_VIEW_ITEM_COUNT / 2, gViewItem.index);
            EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, gViewItem.handle);
            EXPECT_EQ(VIEW_ITEM_DURATION_LARGE, gViewItem.duration);
            EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ((index - MAX_VIEW_ITEM_COUNT / 2) * VIEW_ITEM_DURATION_LARGE, gViewItem.timestamp);
            EXPECT_TRUE(gCurrent);
        }
    }
}

TEST_F(ViewApiFunctionalityTest, OverflowNotificationCallbackDurationTail)
{
    UINT64 index;
    UINT64 timestamp;
    PViewItem pViewItem;

    CreateContentView();

    // Add/check overflow on size
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITEM_COUNT; index++, timestamp += VIEW_ITEM_DURATION_LARGE) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION_LARGE, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetNext(mContentView, &pViewItem));

        if (index < MAX_VIEW_ITEM_COUNT / 2) {
            EXPECT_EQ(0, gCustomData);
            EXPECT_EQ(NULL, gContentView);
            EXPECT_EQ(0, gViewItem.timestamp);
            EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ(0, gViewItem.duration);
            EXPECT_EQ(0, gViewItem.handle);
            EXPECT_EQ(0, gViewItem.index);
            EXPECT_FALSE(gCurrent);
        } else {
            EXPECT_EQ(VIEW_NOTIFICATION_CALLBACK_CUSTOM_DATA, gCustomData);
            EXPECT_EQ(mContentView, gContentView);
            EXPECT_EQ(index - MAX_VIEW_ITEM_COUNT / 2, gViewItem.index);
            EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, gViewItem.handle);
            EXPECT_EQ(VIEW_ITEM_DURATION_LARGE, gViewItem.duration);
            EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(gViewItem.flags));
            EXPECT_EQ((index - MAX_VIEW_ITEM_COUNT / 2) * VIEW_ITEM_DURATION_LARGE, gViewItem.timestamp);
            EXPECT_FALSE(gCurrent);
        }
    }
}

TEST_F(ViewApiFunctionalityTest, RollbackCurrentSimpleVariations)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp;

    CreateContentView();

    // Add/check
    for (timestamp = VIEW_ITEM_DURATION, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(0, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);
    }

    // We should be on the 0th element
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITERATION_COUNT / 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT / 2 - 1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITERATION_COUNT / 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT / 2 - 1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITERATION_COUNT / 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT / 2 - 2, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, MAX_VIEW_ITERATION_COUNT / 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(MAX_VIEW_ITERATION_COUNT / 2 - 2, curIndex);
}

TEST_F(ViewApiFunctionalityTest, RollbackCurrentSimpleVariationsSparse)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp;

    CreateContentView();

    // Add/check
    for (timestamp = VIEW_ITEM_DURATION, index = 0; index < MAX_VIEW_ITERATION_COUNT / 2; index++, timestamp += 2 * VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(0, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        // Make sure it fails when trying to retrieve a timestamp over the newest duration
        EXPECT_NE(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration + 10, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        // Make sure it fails when trying to retrieve a timestamp over the newest duration
        EXPECT_NE(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration + 10, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        if (index != 0) {
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index - 1, &pViewItem));

            // This is a sparse view so we should still return an item even if the timestamp is above the last to newest duration
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration + 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

            // The expected item is in-between the items and will stick with the newest item
            EXPECT_EQ(index, pViewItem->index);

            EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index - 1, &pViewItem));

            // This is a sparse view so we should still return an item even if the timestamp is above the last to newest duration
            EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration + 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

            // The expected item is in-between the items and will stick with the newest item
            EXPECT_EQ(index, pViewItem->index);
        }

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);
    }

    // We should be on the 0th element
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 15));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(14, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 15));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(14, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 15));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(14, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 15));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(14, curIndex);
}

TEST_F(ViewApiFunctionalityTest, RollbackCurrentSimpleVariationsSparseKeyFrame)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    UINT32 fragmentStart;
    PViewItem pViewItem;
    UINT64 timestamp;

    CreateContentView();

    // Add/check
    for (timestamp = VIEW_ITEM_DURATION, index = 0; index < MAX_VIEW_ITERATION_COUNT / 2; index++, timestamp += 2 * VIEW_ITEM_DURATION) {
        // Set a key frame every 7th starting from 2nd
        fragmentStart = (index % 7 == 2) ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, fragmentStart));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(0, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);
    }

    // We should be on the 0th element
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);

    // Set somewhere in the middle
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    // Set at the beginning
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);
}

TEST_F(ViewApiFunctionalityTest, RollbackCurrentSimpleVariationsSparseKeyFrameReceivedAck)
{
    UINT64 index, curIndex;
    BOOL itemExists;
    UINT32 fragmentStart;
    PViewItem pViewItem;
    UINT64 timestamp;

    CreateContentView();

    // Add/check
    for (timestamp = VIEW_ITEM_DURATION, index = 0; index < MAX_VIEW_ITERATION_COUNT / 2; index++, timestamp += 2 * VIEW_ITEM_DURATION) {
        // Set a key frame every 7th starting from 2nd
        fragmentStart = (index % 7 == 2) ? ITEM_FLAG_FRAGMENT_START : ITEM_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, 1, fragmentStart));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pViewItem));
        EXPECT_FALSE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->timestamp);
        EXPECT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        EXPECT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        EXPECT_EQ(0, pViewItem->index);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
        EXPECT_EQ(0, curIndex);

        EXPECT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);

        EXPECT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        EXPECT_TRUE(itemExists);
    }

    // We should be on the 0th element
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 0, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, timestamp, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(0, curIndex);

    // Set the ACKs
    // IMPORTANT!!! The scenario below with all the received ACKs on non-key frames.
    // in real-life case this won't be possible as the acks will be sent for the received fragment.
    // The tests below have this behavior. We will iterate once more with more realistic scenario with ACKs.
    for (index = 0; index <= 20; index++) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));
        SET_ITEM_BUFFERING_ACK(pViewItem->flags);
        SET_ITEM_RECEIVED_ACK(pViewItem->flags);
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(20, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(20, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(20, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(20, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 5 * 2 * VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(9, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 5 * 2 * VIEW_ITEM_DURATION, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(20, curIndex);

    // Set at the beginning
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);

    // Set the ACKs in a more realistic scenario.
    for (index = 16; index <= 20; index++) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));
        CLEAR_ITEM_RECEIVED_ACK(pViewItem->flags);
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(19, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 5 * 2 * VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(9, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 20));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, 5 * 2 * VIEW_ITEM_DURATION, TRUE, TRUE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(16, curIndex);

    // Set at the beginning
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, FALSE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(1, curIndex);

    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 2));
    EXPECT_EQ(STATUS_SUCCESS, contentViewRollbackCurrent(mContentView, VIEW_ITEM_DURATION + 1, TRUE, FALSE));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetCurrentIndex(mContentView, &curIndex));
    EXPECT_EQ(2, curIndex);
}

TEST_F(ViewApiFunctionalityTest, getItemWithTimestamp)
{
    PViewItem pViewItem = NULL, pTail = NULL, pHead = NULL;
    UINT64 index;
    UINT64 timestamp;

    CreateContentView();

    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, 0, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, 0, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

    // Add/check
    for (timestamp = 1, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pHead));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    }

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp + pHead->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp + pTail->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, VIEW_ITEM_DURATION, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp + pHead->duration + 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp + pHead->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp + pTail->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, VIEW_ITEM_DURATION, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, pTail->timestamp - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_TIMESTAMP, contentViewGetItemWithTimestamp(mContentView, pHead->timestamp + pHead->duration + 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
}

TEST_F(ViewApiFunctionalityTest, contentViewTrimTail)
{
    PViewItem pTail = NULL, pHead = NULL;
    UINT64 index;
    UINT64 timestamp;

    CreateContentView();

    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_INDEX, contentViewTrimTail(mContentView, 0));

    // Add/check
    for (timestamp = 0, index = 0; index < MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        EXPECT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
        EXPECT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pHead));

        EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_INDEX, contentViewTrimTail(mContentView, pHead->index + 1));
    }

    EXPECT_EQ(STATUS_CONTENT_VIEW_INVALID_INDEX, contentViewTrimTail(mContentView, pHead->index + 1));

    // trim till 0 - ensure the callback is not called
    EXPECT_EQ(STATUS_SUCCESS, contentViewTrimTail(mContentView, 0));
    EXPECT_EQ(0, gViewItem.index);
    EXPECT_EQ(0, gCallCount);
    EXPECT_FALSE(gCurrent);

    // Ensure tail hasn't been affected
    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
    EXPECT_EQ(0, pTail->index);

    // trim till 1 - ensure the callback is called
    EXPECT_EQ(STATUS_SUCCESS, contentViewTrimTail(mContentView, 1));
    EXPECT_EQ(0, gViewItem.index);
    EXPECT_EQ(1, gCallCount);
    EXPECT_TRUE(gCurrent);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
    EXPECT_EQ(1, pTail->index);

    // Move the current to 5th and trim to 5th
    EXPECT_EQ(STATUS_SUCCESS, contentViewSetCurrentIndex(mContentView, 5));
    EXPECT_EQ(STATUS_SUCCESS, contentViewTrimTail(mContentView, 5));
    EXPECT_EQ(4, gViewItem.index);
    EXPECT_EQ(5, gCallCount);
    EXPECT_FALSE(gCurrent);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
    EXPECT_EQ(5, pTail->index);

    // trim all the way
    EXPECT_EQ(STATUS_SUCCESS, contentViewTrimTail(mContentView, pHead->index));
    EXPECT_EQ(pHead->index - 1, gViewItem.index);
    EXPECT_TRUE(gCurrent);

    EXPECT_EQ(STATUS_SUCCESS, contentViewGetTail(mContentView, &pTail));
    EXPECT_EQ(pHead->index, pTail->index);
}

/**
 * NOTE: Disabling this test as it runs for long time to try to simulate a wraparound for 32 bit.
 * We currently use 64 bit indexes so it will not wrap around at 32 bit boundary.
 */
TEST_F(ViewApiFunctionalityTest, DISABLED_IntOverflowRangeCheck)
{
    UINT64 index;
    BOOL itemExists;
    PViewItem pViewItem;
    UINT64 timestamp = 0;

    CreateContentView();

    // Add/check
    for (timestamp = 0, index = 0; index < (UINT64) MAX_UINT32 + MAX_VIEW_ITERATION_COUNT; index++, timestamp += VIEW_ITEM_DURATION) {
        if (index % 1000000 == 0) {
            DLOGI("View item %llu", index);
        }

        ASSERT_EQ(STATUS_SUCCESS, contentViewAddItem(mContentView, timestamp, timestamp, VIEW_ITEM_DURATION, INVALID_ALLOCATION_HANDLE_VALUE, 0, VIEW_ITEM_ALLOCAITON_SIZE, ITEM_FLAG_FRAGMENT_START));

        ASSERT_EQ(STATUS_SUCCESS, contentViewGetHead(mContentView, &pViewItem));
        ASSERT_TRUE(CHECK_ITEM_FRAGMENT_START(pViewItem->flags));
        ASSERT_EQ(timestamp, pViewItem->timestamp);
        ASSERT_EQ(VIEW_ITEM_DURATION, pViewItem->duration);
        ASSERT_EQ(INVALID_ALLOCATION_HANDLE_VALUE, pViewItem->handle);
        ASSERT_EQ(index, pViewItem->index);

        ASSERT_EQ(STATUS_SUCCESS, contentViewItemExists(mContentView, index, &itemExists));
        ASSERT_TRUE(itemExists);

        ASSERT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        ASSERT_TRUE(itemExists);

        ASSERT_EQ(STATUS_SUCCESS, contentViewTimestampInRange(mContentView, timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &itemExists));
        ASSERT_TRUE(itemExists);

        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemAt(mContentView, index, &pViewItem));

        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));

        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
        ASSERT_EQ(STATUS_SUCCESS, contentViewGetItemWithTimestamp(mContentView, pViewItem->timestamp + pViewItem->duration - 1, !CHECK_AGAINST_ACKTIMESTAMP, &pViewItem));
    }
}
