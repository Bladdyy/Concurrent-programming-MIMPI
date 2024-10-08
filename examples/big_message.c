#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "../mimpi.h"
#include "mimpi_err.h"
#include "test.h"

char data[21372137];

int main(int argc, char **argv)
{
    MIMPI_Init(false);

    int const world_rank = MIMPI_World_rank();

    memset(data, world_rank == 0 ? 42 : 7, sizeof(data));

    int const tag = 17;

    if (world_rank == 0) {
        ASSERT_MIMPI_OK(MIMPI_Send(data, sizeof(data), 1, tag));
        for (int i = 0; i < sizeof(data); i += 789) {
            test_assert(data[789] == 42);
        }
    }
    else if (world_rank == 1)
    {
        ASSERT_MIMPI_OK(MIMPI_Recv(data, sizeof(data), 0, tag));
        for (int i = 0; i < sizeof(data); i += 789) {
            test_assert(data[789] == 42);
        }
    }

    MIMPI_Finalize();
    return test_success();
}
