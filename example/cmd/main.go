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

	token = client.Subscribe("+/Route/#", 0, func(client mqtt.Client, message mqtt.Message) {
		messages <- message
	})
	token.Wait()

	for m := range messages {
		var bytes []byte
		topic := strings.Split(m.Topic(), "/")
		switch topic[0] {
		case "Q":
			var query sip.Query
			err := proto.Unmarshal(m.Payload(), &query)
			if err != nil {
				log.Fatalln(err)
			}

			log.Println(topic, &query)

			var answer sip.Answer
			answer.Response = NewResponse(503)
			answer.Head = query.Head
			answer.Id = query.Id

			bytes, err = proto.Marshal(&answer)
			if err != nil {
				log.Fatalln(err)
			}

			topic[0] = "A"

		default:
			continue
		}

		topic[1] = "Sip"

		log.Println(topic, bytes)

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
