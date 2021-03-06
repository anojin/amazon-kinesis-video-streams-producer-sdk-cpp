#include "ClientTestFixture.h"

class StreamPutGetTest : public ClientTestBase {
};

TEST_F(StreamPutGetTest, putFrame_PutGetFrameBoundary)
{
    UINT32 i, j, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle, availableStorage, curAvailableStorage;
    Frame frame;
    ClientMetrics memMetrics;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Get the storage size to compare
    availableStorage = mDeviceInfo.storageInfo.storageSize;

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    memMetrics.version = CLIENT_METRICS_CURRENT_VERSION;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        // Get the available storage size
        EXPECT_EQ(STATUS_SUCCESS, getKinesisVideoMetrics(mClientHandle, &memMetrics));
        curAvailableStorage = memMetrics.contentStoreAvailableSize;

        // Ensure it's decreased by greater than or equal amount of bytes
        EXPECT_GE(availableStorage - frame.size, curAvailableStorage) << "Failed at iteration " << i;
        availableStorage = curAvailableStorage;
    }

    // Consume frames on the boundary and validate

    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetFrameBoundaryInterleaved)
{
    UINT32 i, j, k, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;
    UINT32 putStreamResultCount = 20;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    k = 0;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        // We will be reading from the beginning only after we have the upload handle
        k = i - putStreamResultCount;

        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (k == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (k % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize);

        if (i >= putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS, retStatus);
            EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
            EXPECT_EQ(bufferSize, filledSize);
        } else {
            // PutStreamResult hasn't been called yet
            EXPECT_EQ(STATUS_END_OF_STREAM, retStatus);
            EXPECT_EQ(INVALID_UPLOAD_HANDLE_VALUE, clientStreamHandle);
            EXPECT_EQ(0, filledSize);
        }

        if (STATUS_SUCCEEDED(retStatus)) {
            // Validate the fill pattern
            validPattern = TRUE;
            for (j = 0; j < SIZEOF(tempBuffer); j++) {
                if (getDataBuffer[offset + j] != k) {
                    validPattern = FALSE;
                    break;
                }
            }

            EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << k;
        }
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetFrameBoundaryInterleavedUnderrun)
{
    UINT32 i, j, k, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;
    UINT32 putStreamResultCount = 20;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    k = 0;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        if (i == putStreamResultCount) {
            // Catch-up with the head and continue
            for (k = 0; k < i; k++) {
                getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer,
                                          SIZEOF(tempBuffer) + GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT), &filledSize);
            }
        } else {
            retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize,
                                                  &filledSize);

            if (i > putStreamResultCount) {
                EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
                EXPECT_EQ(STATUS_SUCCESS, retStatus);
                EXPECT_EQ(bufferSize, filledSize);
            } else {
                // PutStreamResult hasn't been called yet
                EXPECT_EQ(INVALID_UPLOAD_HANDLE_VALUE, clientStreamHandle);
                EXPECT_EQ(STATUS_END_OF_STREAM, retStatus);
                EXPECT_EQ(0, filledSize);
            }

            if (STATUS_SUCCEEDED(retStatus)) {
                // Validate the fill pattern
                validPattern = TRUE;
                for (j = 0; j < SIZEOF(tempBuffer); j++) {
                    if (getDataBuffer[offset + j] != i) {
                        validPattern = FALSE;
                        break;
                    }
                }

                EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;

                if (k >= putStreamResultCount) {
                    // No more data should be available
                    EXPECT_EQ(STATUS_NO_MORE_DATA_AVAILABLE,
                              getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize,
                                                        &filledSize));
                }
            }
        }
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetInterleavedUnderrun)
{
    UINT32 i, j, k, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;
    UINT32 putStreamResultCount = 20;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS,
                      putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(getDataBuffer);

        if (i == putStreamResultCount) {
            // Catch-up with the head and continue
            for (k = 0; k < i; k++) {
                getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer,
                                          SIZEOF(tempBuffer) + GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT), &filledSize);
            }
        } else {
            retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize,
                                                  &filledSize);

            if (i > putStreamResultCount) {
                EXPECT_EQ(STATUS_NO_MORE_DATA_AVAILABLE, retStatus);
                EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
                EXPECT_EQ(SIZEOF(tempBuffer) + offset, filledSize);

                // Validate the fill pattern
                validPattern = TRUE;
                for (j = 0; j < SIZEOF(tempBuffer); j++) {
                    if (getDataBuffer[offset + j] != i) {
                        validPattern = FALSE;
                        break;
                    }
                }

                EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
            } else {
                // PutStreamResult hasn't been called yet
                EXPECT_EQ(STATUS_END_OF_STREAM, retStatus);
                EXPECT_EQ(INVALID_UPLOAD_HANDLE_VALUE, clientStreamHandle);
                EXPECT_EQ(0, filledSize);
            }
        }
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetHalfBuffer)
{
    UINT32 i, j, iterations, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames
    iterations = i;
    for (i = 0; i < iterations; i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + half of frame bits size
        bufferSize = SIZEOF(tempBuffer) / 2 + offset;

        // Read the first half
        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Read the second half
        filledSize = bufferSize;
        bufferSize = SIZEOF(tempBuffer) / 2;
        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer + filledSize, bufferSize,
                                            &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetHalfBufferInterleaved) {
    UINT32 i, j, k, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;
    UINT32 putStreamResultCount = 20;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS,
                      putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }

        // Consume frames
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + half of frame bits size
        bufferSize = SIZEOF(tempBuffer) / 2 + offset;

        // Read the first half
        retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize,
                                              &filledSize);

        if (i == putStreamResultCount) {
            EXPECT_EQ(STATUS_SUCCESS, retStatus);

            // Catch-up with the head and continue
            for (k = 0; k < i; k++) {
                getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer,
                                          SIZEOF(tempBuffer) + GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT), &filledSize);
            }
        } else {
            if (i > putStreamResultCount) {
                EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
                EXPECT_EQ(STATUS_SUCCESS, retStatus);
                EXPECT_EQ(bufferSize, filledSize);
            } else {
                // PutStreamResult hasn't been called yet
                EXPECT_EQ(INVALID_UPLOAD_HANDLE_VALUE, clientStreamHandle);
                EXPECT_EQ(STATUS_END_OF_STREAM, retStatus);
            }

            // Read the second half
            filledSize = bufferSize;
            bufferSize = SIZEOF(tempBuffer) / 2;
            retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer + filledSize,
                                                  bufferSize, &filledSize);

            if (i > putStreamResultCount) {
                EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);
                EXPECT_EQ(STATUS_SUCCESS, retStatus);
                EXPECT_EQ(bufferSize, filledSize);

                // Validate the fill pattern
                validPattern = TRUE;
                for (j = 0; j < SIZEOF(tempBuffer); j++) {
                    if (getDataBuffer[offset + j] != i) {
                        validPattern = FALSE;
                        break;
                    }
                }

                EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;

                // No more data should be available
                EXPECT_EQ(STATUS_NO_MORE_DATA_AVAILABLE,
                          getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize,
                                                    &filledSize));
            } else {
                // PutStreamResult hasn't been called yet
                EXPECT_EQ(INVALID_UPLOAD_HANDLE_VALUE, clientStreamHandle);
                EXPECT_EQ(STATUS_END_OF_STREAM, retStatus);
            }
        }
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetDoubleBuffer)
{
    UINT32 i, j, iterations, filledSize, offset1, offset2, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[5000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames
    iterations = i / 2;
    for (i = 0; i < iterations; i+=2) {
        clientStreamHandle = 0;

        // The first two frames will have the simple block, cluster and MKV overhead
        if (i == 0) {
            offset1 = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
            offset2 = MKV_SIMPLE_BLOCK_OVERHEAD;
        } else if (i % 10 == 0) {
            // Cluster start will have simple block and cluster overhead
            offset1 = MKV_CLUSTER_OVERHEAD;
            offset2 = MKV_SIMPLE_BLOCK_OVERHEAD;
        } else {
            // Two simple block overheads
            offset1 = offset2 = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + half of frame bits size
        bufferSize = SIZEOF(tempBuffer) * 2 + offset1 + offset2;

        // Read the double
        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset1 + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at first half offset: " << j << " from the beginning of frame: " << i;

        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset1 + SIZEOF(tempBuffer) + offset2 + j] != i + 1) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at second half offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetNonBoundaryBuffer)
{
    UINT32 i, j, iterations, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[5000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames - half on the boundary
    iterations = i / 2;
    for (i = 0; i < iterations; i++) {
        clientStreamHandle = 0;

        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Two simple block overheads
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        bufferSize = SIZEOF(tempBuffer) + offset;

        // Read the double
        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at first half offset: " << j << " from the beginning of frame: " << i;
    }

    // Read 1.5 frame size
    bufferSize = SIZEOF(tempBuffer) + SIZEOF(tempBuffer) / 2 + MKV_CLUSTER_OVERHEAD + MKV_SIMPLE_BLOCK_OVERHEAD;
    EXPECT_EQ(STATUS_SUCCESS,
              getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
    EXPECT_EQ(bufferSize, filledSize);

    // Validate the first part fill pattern
    validPattern = TRUE;
    for (j = 0; j < SIZEOF(tempBuffer); j++) {
        if (getDataBuffer[MKV_CLUSTER_OVERHEAD + j] != i) {
            validPattern = FALSE;
            break;
        }
    }

    EXPECT_TRUE(validPattern) << "Failed at first half offset: " << j << " from the beginning of frame: " << i;

    // validate the other half - should be the half of the next frame.
    validPattern = TRUE;
    i++;
    for (j = 0; j < SIZEOF(tempBuffer) / 2; j++) {
        if (getDataBuffer[SIZEOF(tempBuffer) + MKV_CLUSTER_OVERHEAD + MKV_SIMPLE_BLOCK_OVERHEAD + j] != i) {
            validPattern = FALSE;
            break;
        }
    }

    EXPECT_TRUE(validPattern) << "Failed at first half offset: " << j << " from the beginning of frame: " << i;

}

TEST_F(StreamPutGetTest, putFrame_PutGetNonKeyFrameFirstFrame)
{
    UINT32 i, j, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th with the exception of the first
        frame.flags = (i % 10 == 0 && i != 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate

    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT);
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetNonKeyFrameFirstFrameCpd1Byte)
{
    UINT32 i, j, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE getDataBuffer[2000];
    BYTE cpd[10];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Set a CPD
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateData = cpd;
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateDataSize = SIZEOF(cpd);

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th with the exception of the first
        frame.flags = (i % 10 == 0 && i != 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate

    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            // CPD + CPD elem size + CPD encoded len
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT) + SIZEOF(cpd) + MKV_CODEC_PRIVATE_DATA_ELEM_SIZE + 1;
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetNonKeyFrameFirstFrameCpd2Byte)
{
    UINT32 i, j, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE cpd[0x4000 - 2];
    BYTE getDataBuffer[2000 + SIZEOF(cpd)];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Set a CPD
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateData = cpd;
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateDataSize = SIZEOF(cpd);

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th with the exception of the first
        frame.flags = (i % 10 == 0 && i != 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate

    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            // CPD + CPD elem size + CPD encoded len
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT) + SIZEOF(cpd) + MKV_CODEC_PRIVATE_DATA_ELEM_SIZE + 2;
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetNonKeyFrameFirstFrameCpd3Byte)
{
    UINT32 i, j, filledSize, offset, bufferSize;
    BOOL validPattern;
    BYTE tempBuffer[1000];
    BYTE cpd[0x4000];
    BYTE getDataBuffer[2000 + SIZEOF(cpd)];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Set a CPD
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateData = cpd;
    mStreamInfo.streamCaps.trackInfoList[TEST_TRACKID].codecPrivateDataSize = SIZEOF(cpd);

    // Create and ready a stream
    ReadyStream();

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th with the exception of the first
        frame.flags = (i % 10 == 0 && i != 0) ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame));

        // Ensure put stream is called
        EXPECT_EQ(1, mPutStreamFuncCount);
        EXPECT_EQ(0, STRCMP(TEST_STREAM_NAME, mStreamName));
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_ENDPOINT, mStreamingEndpoint));
        EXPECT_EQ(TRUE, mAckRequired);
        // Need to ensure we have a streaming token in the auth
        EXPECT_EQ(SIZEOF(TEST_STREAMING_TOKEN), mCallContext.pAuthInfo->size);
        EXPECT_EQ(0, STRCMP(TEST_STREAMING_TOKEN, (PCHAR) mCallContext.pAuthInfo->data));

        // Drop frame hasn't been called
        EXPECT_EQ(0, mFrameTime);

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate

    for (i = 0, timestamp = 0; timestamp < TEST_BUFFER_DURATION; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        clientStreamHandle = 0;

        // The first frame will have the cluster and MKV overhead
        if (i == 0) {
            // CPD + CPD elem size + CPD encoded len
            offset = GET_MKV_HEADER_OVERHEAD(TEST_TRACK_COUNT) + SIZEOF(cpd) + MKV_CODEC_PRIVATE_DATA_ELEM_SIZE + 3;
        } else if (i % 10 == 0) {
            // Cluster start will have cluster overhead
            offset = MKV_CLUSTER_OVERHEAD;
        } else {
            // Simple block overhead
            offset = MKV_SIMPLE_BLOCK_OVERHEAD;
        }

        // Set the buffer size to be the offset + frame bits size
        bufferSize = SIZEOF(tempBuffer) + offset;

        EXPECT_EQ(STATUS_SUCCESS,
                  getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, bufferSize, &filledSize));
        EXPECT_EQ(bufferSize, filledSize);
        EXPECT_EQ(TEST_STREAMING_HANDLE, clientStreamHandle);

        // Validate the fill pattern
        validPattern = TRUE;
        for (j = 0; j < SIZEOF(tempBuffer); j++) {
            if (getDataBuffer[offset + j] != i) {
                validPattern = FALSE;
                break;
            }
        }

        EXPECT_TRUE(validPattern) << "Failed at offset: " << j << " from the beginning of frame: " << i;
    }
}

TEST_F(StreamPutGetTest, putFrame_PutGetTagsStoreData)
{
    UINT32 i, filledSize;
    BYTE tempBuffer[100];
    PBYTE getDataBuffer = NULL;
    UINT32 getDataBufferSize = 500000;
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    getDataBuffer = (PBYTE) MEMALLOC(getDataBufferSize);

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; i < 100; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame)) << "Iteration " << i;

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "postTagName", (PCHAR) "postTagValue", FALSE)) << i;

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate
    retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, getDataBufferSize, &filledSize);
    ASSERT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE);

    // Manually pre-validated data file size
    EXPECT_EQ(18474, filledSize);

    // Store the data in a file
    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_put_get_tags.mkv", TRUE, getDataBuffer, filledSize));
    MEMFREE(getDataBuffer);
}

TEST_F(StreamPutGetTest, putFrame_PutGetPreTagsStoreData)
{
    UINT32 i, filledSize;
    BYTE tempBuffer[100];
    PBYTE getDataBuffer = NULL;
    UINT32 getDataBufferSize = 500000;

    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    getDataBuffer = (PBYTE) MEMALLOC(getDataBufferSize);

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; i < 100; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Add tag every 3rd frame
        if (i % 3 == 0) {
            EXPECT_EQ(STATUS_SUCCESS,
                      putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "preTagName", (PCHAR) "preTagValue", FALSE)) << i;
        }

        // Key frame every 10th
        frame.flags = i % 10 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame)) << "Iteration " << i;

        // Add tag every 3rd frame
        if (i % 3 == 0) {
            EXPECT_EQ(STATUS_SUCCESS,
                      putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "postTagName", (PCHAR) "postTagValue", FALSE)) << i;
        }

        // Return a put stream result on 20th
        if (i == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Consume frames on the boundary and validate
    retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, getDataBufferSize, &filledSize);
    ASSERT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE);

    // Manually pre-validated data file size
    EXPECT_EQ(16237, filledSize);

    // Store the data in a file
    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_put_get_pre_tags.mkv", TRUE, getDataBuffer, filledSize));

    MEMFREE(getDataBuffer);
}

TEST_F(StreamPutGetTest, putFrame_PutGetTagsBeforeStoreData)
{
    UINT32 i, filledSize;
    BYTE tempBuffer[100];
    PBYTE getDataBuffer = NULL;
    UINT32 getDataBufferSize = 500000;
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    getDataBuffer = (PBYTE) MEMALLOC(getDataBufferSize);

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);

    // Insert a tag first - these should just accumulate
    EXPECT_EQ(STATUS_SUCCESS,
              putFragmentMetadata(pKinesisVideoStream, (PCHAR) "prePrependTagName1", (PCHAR) "prePrependTagValue1", FALSE));

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; i < 20; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        // Insert before a key frame
        if ((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) {
            EXPECT_EQ(STATUS_SUCCESS,
                      putFragmentMetadata(pKinesisVideoStream, (PCHAR) "AppendTagName1", (PCHAR) "AppendTagValue1", FALSE)) << i;
            EXPECT_EQ(STATUS_SUCCESS,
                      putFragmentMetadata(pKinesisVideoStream, (PCHAR) "AppendTagName2", (PCHAR) "AppendTagValue2", FALSE)) << i;
        }

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame)) << "Iteration " << i;
        EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "PrependTagName1", (PCHAR) "PrependTagValue1", FALSE)) << i;
        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFragmentMetadata(mStreamHandle, (PCHAR) "postTagName", (PCHAR) "postTagValue", FALSE)) << i;

        // Return a put stream result on 10th
        if (i == 10) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Append a tag
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "Should not appear", (PCHAR) "Should not appear", FALSE));

    // Consume frames on the boundary and validate
    retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, getDataBufferSize, &filledSize);
    ASSERT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE);

    // Manually pre-validated data file size
    EXPECT_EQ(6697, filledSize);

    // Store the data in a file
    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_insert_pre_tags.mkv", TRUE, getDataBuffer, filledSize));

    MEMFREE(getDataBuffer);
}

TEST_F(StreamPutGetTest, putFrame_PutGetPersistentTagsStoreData)
{
    UINT32 i, filledSize;
    BYTE tempBuffer[100];
    PBYTE getDataBuffer = NULL;
    UINT32 getDataBufferSize = 500000;
    CHAR tagValue[1000];
    UINT64 timestamp, clientStreamHandle;
    Frame frame;
    STATUS retStatus;

    // Ensure we have fragmentation based on the key frames
    mStreamInfo.streamCaps.keyFrameFragmentation = TRUE;

    // Create and ready a stream
    ReadyStream();

    getDataBuffer = (PBYTE) MEMALLOC(getDataBufferSize);

    PKinesisVideoStream pKinesisVideoStream = FROM_STREAM_HANDLE(mStreamHandle);

    // put a persistent tag multiple times and a non-persistent one - should have two
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName0", (PCHAR) "tagValue0", TRUE));
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName0", (PCHAR) "tagValue0", TRUE));
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName0", (PCHAR) "tagValue0", TRUE));
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName0", (PCHAR) "tagValue0", FALSE));

    // Produce frames
    frame.duration = TEST_LONG_FRAME_DURATION;
    frame.size = SIZEOF(tempBuffer);
    frame.frameData = tempBuffer;
    frame.trackId = TEST_TRACKID;
    for (i = 0, timestamp = 0; i < 30; timestamp += TEST_LONG_FRAME_DURATION, i++) {
        frame.index = i + 1;
        frame.decodingTs = timestamp;
        frame.presentationTs = timestamp;

        // Set the frame bits
        MEMSET(frame.frameData, (BYTE) i, SIZEOF(tempBuffer));

        // Key frame every 3rd
        frame.flags = i % 3 == 0 ? FRAME_FLAG_KEY_FRAME : FRAME_FLAG_NONE;

        EXPECT_EQ(STATUS_SUCCESS, putKinesisVideoFrame(mStreamHandle, &frame)) << "Iteration " << i;

        if ((frame.flags & FRAME_FLAG_KEY_FRAME) == FRAME_FLAG_KEY_FRAME) {
            // Modify the name of a tag
            sprintf(tagValue, "persistentValue%d", frame.index);
            EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "persistentName", (PCHAR) tagValue, TRUE)) << i;
        }

        if (frame.index % 9 == 0) {
            sprintf(tagValue, "tagValue%d", frame.index);
            EXPECT_EQ(STATUS_SUCCESS,
                      putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName1", (PCHAR) "nonPersistentTagValue", FALSE)) << i;
            EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName1", tagValue, TRUE)) << i;

            EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName2", tagValue, TRUE)) << i;
        }

        // Remove the tag
        if (frame.index == 20) {
            EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "tagName2", (PCHAR) "", TRUE)) << i;
        }

        // Return a put stream result on 10th
        if (i == 10) {
            EXPECT_EQ(STATUS_SUCCESS, putStreamResultEvent(mCallContext.customData, SERVICE_CALL_RESULT_OK, TEST_STREAMING_HANDLE));
        }
    }

    // Append a tag
    EXPECT_EQ(STATUS_SUCCESS, putFragmentMetadata(pKinesisVideoStream, (PCHAR) "Should not appear", (PCHAR) "Should not appear", FALSE));

    // Consume frames on the boundary and validate
    retStatus = getKinesisVideoStreamData(mStreamHandle, &clientStreamHandle, getDataBuffer, getDataBufferSize, &filledSize);
    ASSERT_TRUE(retStatus == STATUS_SUCCESS || retStatus == STATUS_NO_MORE_DATA_AVAILABLE);

    // Manually pre-validated data file size
    EXPECT_EQ(6394, filledSize);

    // Store the data in a file
    EXPECT_EQ(STATUS_SUCCESS, writeFile((PCHAR) "test_insert_persistent_tags.mkv", TRUE, getDataBuffer, filledSize));

    MEMFREE(getDataBuffer);
}
