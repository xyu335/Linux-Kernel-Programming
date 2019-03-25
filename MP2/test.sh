#!/bin/sh

echo "Test case A, single userapp"
echo "Params:"
echo "period:500ms\ncomputation:2 compute function calls(43ms * 2)\ntask_times-3"
echo "Expected Result: The collective utilization of the period is 86/500 < 0.693, task should work fine."
echo "Start test A"
./userapp 500 2 3

echo "\n"
echo "Test case B, two userapp"
echo "period:500ms\ncomputation:2 compute function calls(43ms * 2)\ntask_times-3"
echo "period:100ms\ncomputation:2 compute function calls(43ms * 2)\ntask_times-2"
echo "Expected Result: Second task should not pass the admission control. first task should work well."
echo "Start test B"
./userpp 500 2 3 & ./userapp 100 2 2

echo "\n"
echo "Test case C, two userapp"
echo "period:500ms\ncomputation:2 compute function calls(43ms * 2)\ntask_times-3"
echo "period:300ms\ncomputation:1 compute function calls(43ms * 1)\ntask_times-7"
echo "Expected Result: The collective utilization of the period is 86/500 + 43/300 < 0.693, two task should work fine. "
echo "Second task should live longer than the first."
echo "Start test C"
./userpp 500 2 3 & ./userapp 300 1 7

