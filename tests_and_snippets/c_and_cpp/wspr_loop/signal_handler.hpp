#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <csignal>
#include <iostream>

extern void block_signals();
extern void cleanup_threads();
extern void signal_handler(int);
extern void register_signal_handlers();
extern void suppress_terminal_signals();

#endif // SIGNAL_HANDLER_HPP
