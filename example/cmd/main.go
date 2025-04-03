package main

import (
	"log"
	"strings"

	"github.com/eclipse/paho.mqtt.golang"

	"google.golang.org/protobuf/proto"

	"github.com/pshvedko/sipecho/example/sip"
)

func main() {
	messages := make(chan mqtt.Message)

	opts := mqtt.NewClientOptions()
	opts.AddBroker("tcp://uru:1883")
	opts.SetClientID("route")

	client := mqtt.NewClient(opts)
	token := client.Connect()
	token.Wait()

	token = client.Subscribe("Route/#", 0, func(client mqtt.Client, message mqtt.Message) {
		messages <- message
	})
	token.Wait()

	for m := range messages {
		topic := strings.Split(m.Topic(), "/")

		var query sip.Message
		err := proto.Unmarshal(m.Payload(), &query)
		if err != nil {
			log.Fatalln(err)
		}

		log.Println(">>>", topic, &query)

		var reply sip.Message
		reply.Response = NewResponse(200)
		reply.Head = query.Head
		reply.Id = query.Id

		topic[0] = "Sip"

		log.Println("<<<", topic, &reply)

		bytes, err := proto.Marshal(&reply)
		if err != nil {
			log.Fatalln(err)
		}

		token = client.Publish(strings.Join(topic, "/"), 0, false, bytes)
		token.Wait()
	}
}

func NewResponse(i int32) *int32 {
	return Ptr(i)
}

func Ptr[T any](v T) *T {
	return &v
}
