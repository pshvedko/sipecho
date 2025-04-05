package sip

import (
	"testing"

	"github.com/google/uuid"

	"github.com/stretchr/testify/require"
)

func TestNewUUID(t *testing.T) {
	type args struct {
		uu uuid.UUID
	}
	tests := []struct {
		name string
		args args
	}{
		// TODO: Add test cases.
		{
			name: "",
			args: args{uu: uuid.New()},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			require.Equal(t, tt.args.uu, NewUUIDFrom(tt.args.uu).GetUUID())
		})
	}
}
