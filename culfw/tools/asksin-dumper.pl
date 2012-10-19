#!/usr/bin/perl

use warnings;
use strict;

use Time::HiRes;

use POE;
use POE::Wheel::ReadWrite;
use POE::Wheel::ReadLine;

use Symbol qw(gensym);
use Device::SerialPort;
use DateTime;

my $device = shift @ARGV || '/dev/ttyACM0';

my $loop = 0;

POE::Session->create(
		     inline_states => {
				       _start        => \&setup_device,
				       got_message   => \&got_message,
				       got_console   => \&transmit_console_data,
				       got_error     => \&handle_errors,
				      },
		    );

POE::Kernel->run();
exit 0;

sub setup_device {
  my ( $kernel, $heap ) = @_[ KERNEL, HEAP ];
  
  # Open a serial port, and tie it to a file handle for POE.
  
  my $handle = gensym();
  my $port = tie( *$handle, "Device::SerialPort", $device );
  die "can't open port: $!" unless $port;
  
  $port->baudrate(38400);
  $port->databits(8);
  $port->parity("none");
  $port->stopbits(1);

  $heap->{port}       = $port;
  $heap->{port_wheel} = POE::Wheel::ReadWrite->new(
						   Handle => $handle,
						   Filter => POE::Filter::AskSin->new(
										     ),
						   InputEvent => "got_message",
						   ErrorEvent => "got_error",
						  );

  # Start a wheel to interact with the console, and prompt the user.
  $heap->{console} = POE::Wheel::ReadLine->new(
					       InputEvent => "got_console",
					      );
  
  $heap->{console}->put("Press ^D to stop.");
  $heap->{console}->get("Ready: ");

  $heap->{port_wheel}->put( 'V' );
  $heap->{port_wheel}->put( 'X10' );
  $heap->{port_wheel}->put( 'Ar' );
}

sub got_message {
  my ( $heap, $data ) = @_[ HEAP, ARG0 ];

  return unless @$data;

  my $msg = sprintf '%s nr: %02X cc: %02X ty: %02X s: %02X%02X%02X d: %02X%02X%02X pl: ', DateTime->now()->strftime('%T'), @$data;

  foreach (9..@$data-1) {
    $msg .= sprintf '%02X ', $data->[$_]
  }
  
  foreach (9..@$data-1) {
    $msg .= sprintf '%s', ($data->[$_]<127 && $data->[$_]>30) ? chr($data->[$_]) : '.';
  }
  
  $heap->{console}->put( $msg );

  $! = 0;
}


# Port data (lines, separated by CRLF) are displayed on the console.
sub data_message {
  my ( $heap, $data ) = @_[ HEAP, ARG0 ];

  return unless ref $data;

  $heap->{console}->put( $data->dump() );
}

# Console input is sent to the device.

sub transmit_console_data {
  my ( $heap, $input ) = @_[ HEAP, ARG0 ];
  
  if ( defined $input ) {
    $heap->{console}->addhistory($input);

    $heap->{port_wheel}->put( $input );

    $heap->{console}->get("Ready: ");
    
    return;
  }
  
  $heap->{console}->put("Bye!");
  delete $heap->{port_wheel};
  delete $heap->{console};
}

# Error on the serial port.  Shut down.

sub handle_errors {
  my $heap = $_[HEAP];
  
  $heap->{console}->put("$_[ARG0] error $_[ARG1]: $_[ARG2]");
  $heap->{console}->put("bye!");
  
  delete $heap->{console};
  delete $heap->{port_wheel};
}

#########################################
#
package POE::Filter::AskSin;
use strict;

#------------------------------------------------------------------------------

sub new {
  my $type = shift;
  my $hash = {
	      BUF => '',
	     };

  my $self = bless $hash, $type;
  $self;
}

#------------------------------------------------------------------------------

sub get {
  my ($self, $stream) = @_;

  my (@msgs);

  $self->{BUF} .= join('', @$stream);

  while (1) {
    my $one = $self->get_one();
    last unless @$one;
    push @msgs, shift @$one;
  }

  \@msgs;
}

sub get_one_start {
  my ($self, $stream) = @_;
  $self->{BUF} .= join('', @$stream);
}

sub get_one {
  my $self = shift;

  # find sync from the beginning
  #
  $self->{BUF} =~ s/^[^a]+//i;
  
  my @BYTES = ();

  if ($self->{BUF} =~ /^(a)([\x00-\xff]+)/i) {
    my $mode = $1;
    my $data = $2;

    if ($mode eq 'a') {
      # Binmode
      my $len = ord( substr( $data, 0, 1 ) );

      substr( $data, 0, 1 ) = '';

      if ($len<=length($data)) {

	foreach (0..$len-1) {
	  $BYTES[$_] = ord( substr( $data, $_, 1) );
	}
	
	substr( $self->{BUF}, 0, $len+2 ) = '';
	return [\@BYTES];
      }

    } else {
      # Hexmode
      my $len = hex( substr( $data, 0, 2 ) );

      my @bytes = unpack("(A2)*", substr( $data, 2 ) );
      
      if ($len<=scalar(@bytes)) {
	  
	foreach (0..$len-1) {
	  $BYTES[$_] = hex( $bytes[$_] );
	}
	  
	substr( $self->{BUF}, 0, ($len+1)*2+1 ) = '';
	return [\@BYTES];
      }
      
    }

  }

  return [];
}

#------------------------------------------------------------------------------

sub put {
  my ($self, $blocks) = @_;
  my @raw;

  foreach my $obj (@$blocks) {
    push @raw, "$obj\r\n";
  }


  \@raw;
}

#------------------------------------------------------------------------------

sub get_pending {
  my $self = shift;
  return undef unless length $self->{BUF};
  [ $self->{BUF} ];
}

###############################################################################
1;
__END__
  
