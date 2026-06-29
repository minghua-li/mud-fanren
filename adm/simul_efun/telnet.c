#include <telnet.h>

varargs nomask int input_to(mixed fun, mixed a, mixed b, mixed c, mixed d, mixed e)
{
    int ret;
    object tp = this_player();

    if(intp(a))
        ret = efun::input_to(fun, a, b, c, d, e);
    else
        ret = efun::input_to(fun, 0, a, b, c, d, e);

    if(tp && interactive(tp))
        catch(tp->do_ga());

    return ret;
}

varargs nomask int get_char(mixed fun, mixed a, mixed b)
{
    int ret;
    object tp = this_player();

    if(intp(a))
        ret = efun::get_char(fun, a, b);
    else
        ret = efun::get_char(fun, 0, a, b);

    if(tp && interactive(tp))
        catch(tp->do_ga());

    return ret;
}
