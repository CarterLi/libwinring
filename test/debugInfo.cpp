#include "common.hpp"

int main() {
    win_ring_capabilities capabilities;
    if (win_ring_query_capabilities(&capabilities) < 0) panic();

    printf("IoRing Version: %d\n", (int)capabilities.IoRingVersion);
    printf("Max opcode: %d\n", capabilities.MaxOpCode);
    printf("Supported flags: %x\n\n", capabilities.FlagsSupported);
    printf("Submission queue size: %u\n\n", capabilities.SubmissionQueueSize);
    printf("Completion queue size: %u\n\n", capabilities.CompletionQueueSize);
}