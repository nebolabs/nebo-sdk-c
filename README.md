# Nebo C SDK

Official C SDK for building [Nebo](https://nebo.bot) apps.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Quick Start

```c
#include <nebo/nebo.h>

static int my_execute(const char *input_json, char **output, int *is_error) {
    *output = strdup("Hello from C!");
    *is_error = 0;
    return 0;
}

static const char *actions[] = {"greet", NULL};

int main(void) {
    nebo_schema_builder_t *sb = nebo_schema_new(actions);
    nebo_schema_string(sb, "name", "Who to greet", 1);
    char *schema = nebo_schema_build(sb);
    nebo_schema_free(sb);

    nebo_tool_handler_t tool = {
        .name = "greeter",
        .description = "Says hello.",
        .schema = schema,
        .execute = my_execute,
    };

    nebo_app_t *app = nebo_app_new();
    nebo_app_register_tool(app, &tool);
    int ret = nebo_app_run(app);
    free(schema);
    return ret;
}
```

## Documentation

See [Creating Nebo Apps](https://developer.neboloop.com/docs/creating-apps) for the full guide.
